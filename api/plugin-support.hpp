/* Copyright (c) 2015 Stanislaw Halik
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#pragma once

#include "plugin-api.hpp"
#include "compat/library-path.hpp"

#include <memory>
#include <algorithm>
#include <cstring>

#include <QDebug>
#include <QString>
#include <QLibrary>
#include <QList>
#include <QDir>
#include <QList>
#include <QIcon>

extern "C" {
    using module_ctor_t = void* (*)(void);
    using module_metadata_t = Metadata_* (*)(void);
}

struct dylib final
{
    enum Type : unsigned
    {
        Filter = 0xdeadbabe,
        Tracker = 0xcafebeef,
        Protocol = 0xdeadf00d,
        Extension = 0xcafebabe,
        Invalid = (unsigned)-1,
    };

    dylib(const QString& filename_, Type t) :
        full_filename(filename_),
        module_name(trim_filename(filename_))
    {
        // otherwise dlopen opens the calling executable
        if (filename_.isEmpty() || module_name.isEmpty())
            return;

        handle.setFileName(filename_);
        handle.setLoadHints(QLibrary::DeepBindHint | QLibrary::PreventUnloadHint | QLibrary::ResolveAllSymbolsHint);

#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wcomma"
#endif

        if (check(!handle.load()))
            return;

        if (check((Dialog = (module_ctor_t) handle.resolve("GetDialog"), !Dialog)))
            return;

        if (check((Constructor = (module_ctor_t) handle.resolve("GetConstructor"), !Constructor)))
            return;

        if (check((Meta = (module_metadata_t) handle.resolve("GetMetadata"), !Meta)))
            return;

        std::unique_ptr<Metadata_> m{Meta()};

        icon = m->icon();
        name = m->name();

        type = t;
#ifdef __clang__
#   pragma clang diagnostic pop
#endif
    }

    // QLibrary refcounts the .dll's so don't forcefully unload
    ~dylib() = default;

    static QList<std::shared_ptr<dylib>> enum_libraries(const QString& library_path)
    {
        QDir module_directory(library_path);
        QList<std::shared_ptr<dylib>> ret;

        using str = QLatin1String;

        const struct filter_ {
            Type type{Invalid};
            QLatin1String glob;
        } filters[] = {
            { Filter, str(OPENTRACK_LIBRARY_PREFIX "opentrack-filter-*." OPENTRACK_LIBRARY_EXTENSION), },
            { Tracker, str(OPENTRACK_LIBRARY_PREFIX "opentrack-tracker-*." OPENTRACK_LIBRARY_EXTENSION), },
            { Protocol, str(OPENTRACK_LIBRARY_PREFIX "opentrack-proto-*." OPENTRACK_LIBRARY_EXTENSION), },
            { Extension, str(OPENTRACK_LIBRARY_PREFIX "opentrack-ext-*." OPENTRACK_LIBRARY_EXTENSION), },
        };

        for (const filter_& filter : filters)
        {
            for (const QString& filename : module_directory.entryList({ filter.glob }, QDir::Files, QDir::Name))
            {
                std::shared_ptr<dylib> lib = std::make_shared<dylib>(QStringLiteral("%1/%2").arg(library_path).arg(filename), filter.type);

                if (lib->type == Invalid)
                    continue;

                if (std::any_of(ret.cbegin(),
                                ret.cend(),
                                [&lib](const std::shared_ptr<dylib>& a) {
                                    return a->type == lib->type && a->name == lib->name;
                                }))
                {
                    qDebug() << "duplicate lib" << filename << "ident" << lib->name;
                    continue;
                }

                ret.push_back(lib);
            }
        }

        return ret;
    }

    Type type{Invalid};
    QString full_filename;
    QString module_name;

    QIcon icon;
    QString name;

    module_ctor_t Dialog{nullptr};
    module_ctor_t Constructor{nullptr};
    module_metadata_t Meta{nullptr};
private:
    QLibrary handle;

    static QString trim_filename(const QString& in_)
    {
        QStringRef in(&in_);

        const int idx = in.lastIndexOf("/");

        if (idx != -1)
        {
            in = in.mid(idx + 1);

            if (in.startsWith(OPENTRACK_LIBRARY_PREFIX) &&
                in.endsWith("." OPENTRACK_LIBRARY_EXTENSION))
            {
                constexpr unsigned pfx_len = sizeof(OPENTRACK_LIBRARY_PREFIX) - 1;
                constexpr unsigned rst_len = sizeof("." OPENTRACK_LIBRARY_EXTENSION) - 1;

                in = in.mid(pfx_len);
                in = in.left(in.size() - rst_len);

                const char* const names[] =
                {
                    OPENTRACK_LIBRARY_PREFIX "opentrack-tracker-",
                    OPENTRACK_LIBRARY_PREFIX "opentrack-proto-",
                    OPENTRACK_LIBRARY_PREFIX "opentrack-filter-",
                    OPENTRACK_LIBRARY_PREFIX "opentrack-ext-",
                };

                for (auto name : names)
                {
                    if (in.startsWith(name))
                        return in.mid(std::strlen(name)).toString();
                }
            }
        }
        return {""};
    }

    bool check(bool fail)
    {
        if (fail)
        {
            qDebug() << "library" << module_name << "failed:" << handle.errorString();

            Constructor = nullptr;
            Dialog = nullptr;
            Meta = nullptr;

            type = Invalid;
        }

        return fail;
    }
};

struct Modules final
{
    using dylib_ptr = std::shared_ptr<dylib>;
    using dylib_list = QList<dylib_ptr>;

    Modules(const QString& library_path) :
        module_list(dylib::enum_libraries(library_path)),
        filter_modules(filter(dylib::Filter)),
        tracker_modules(filter(dylib::Tracker)),
        protocol_modules(filter(dylib::Protocol)),
        extension_modules(filter(dylib::Extension))
    {}
    dylib_list& filters() { return filter_modules; }
    dylib_list& trackers() { return tracker_modules; }
    dylib_list& protocols() { return protocol_modules; }
    dylib_list& extensions() { return extension_modules; }

private:
    dylib_list module_list;
    dylib_list filter_modules;
    dylib_list tracker_modules;
    dylib_list protocol_modules;
    dylib_list extension_modules;

    static dylib_list& sorted(dylib_list& xs)
    {
        std::sort(xs.begin(), xs.end(), [&](const dylib_ptr& a, const dylib_ptr& b) { return a->name.toLower() < b->name.toLower(); });
        return xs;
    }

    dylib_list filter(dylib::Type t)
    {
        QList<std::shared_ptr<dylib>> ret;
        for (const auto& x : module_list)
            if (x->type == t)
                ret.push_back(x);

        return sorted(ret);
    }
};

template<typename t>
std::shared_ptr<t> make_dylib_instance(const std::shared_ptr<dylib>& lib)
{
    std::shared_ptr<t> ret;
    if (lib != nullptr && lib->Constructor)
        ret = std::shared_ptr<t>(reinterpret_cast<t*>(reinterpret_cast<module_ctor_t>(lib->Constructor)()));
    return ret;
}
