#pragma once

#ifndef _WINDEF_
struct HINSTANCE__;
typedef HINSTANCE__ *HINSTANCE;
struct HDC__;
typedef HDC__ *HDC;
struct HWND__;
typedef HWND__ *HWND;
#endif

namespace no {

namespace platform {

namespace windows {

HINSTANCE current_instance();
int show_command();

}

}

}
