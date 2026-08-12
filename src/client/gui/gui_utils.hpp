#pragma once

enum class alignment {
    left,
    center,
    right
};

struct nine_patch {
    int tl, t, tr, l, c, r, bl, b, br;
};
