#pragma once

struct InputState
{
    bool driveForward = false;
    bool driveBackward = false;

    bool leanBackward = false;
    bool leanForward = false;

    bool resetPressed = false;

    bool jumpPressed = false;
};