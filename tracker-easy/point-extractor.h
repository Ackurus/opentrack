/*
 * Copyright (c) 2019 Stephane Lenclud
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#pragma once

#include "settings.h"
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>


namespace EasyTracker
{

    const int KPointCount = 3;

    class PointExtractor 
    {
    public:
        PointExtractor();
        // extracts points from frame and draws some processing info into frame, if draw_output is set
        // dt: time since last call in seconds
        void ExtractPoints(const cv::Mat& aFrame, cv::Mat* aPreview, std::vector<cv::Point>& aPoints);
        
        // Settings
        Settings s;
        // Our frame with a channel size of 8 bits
        cv::Mat iFrameChannelSizeOne;
        // Our frame with a single 8 bits channel
        cv::Mat iFrameGray;
    };

}

