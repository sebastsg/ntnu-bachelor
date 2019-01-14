#pragma once

namespace no {

class window;

namespace imgui {

void create(no::window& window);
void destroy();
void start_frame();
void end_frame();
void draw();

}

}
