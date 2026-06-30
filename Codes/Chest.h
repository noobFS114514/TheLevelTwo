#pragma once

#include "GameObject.h"

class Chest : public GameObject {
protected:
    int health = 3;

public:
    void setHealth(int value) {
        health = value;
    }

    ~Chest() noexcept override = default;
};
