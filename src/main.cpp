#include "Application.h"
#include <Arduino.h>

growbed::Application app;

void setup()
{
    app.init();
}

void loop()
{
    app.tick();
}
