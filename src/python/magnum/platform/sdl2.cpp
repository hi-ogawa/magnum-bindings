/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <pybind11/pybind11.h>
#include <Magnum/Platform/Sdl2Application.h>

#include "Corrade/Python.h"

#include "magnum/bootstrap.h"
#include "magnum/platform/application.h"

namespace magnum { namespace platform {

namespace {
    int argc = 0;
}

void sdl2(py::module& m) {
    m.doc() = "SDL2-based platform integration";

    struct PublicizedApplication: Platform::Application {
        explicit PublicizedApplication(const Configuration& configuration, const GLConfiguration& glConfiguration): Platform::Application{Arguments{argc, nullptr}, configuration, glConfiguration} {}

        /* MSVC dies with "error C3640: a referenced or virtual member function
           of a local class must be defined" if this is just `= 0` here. Since
           we're overriding this method below anyway, it doesn't have to be
           pure virtual. */
        #ifdef _MSC_VER
        void drawEvent() override {}
        #else
        void drawEvent() override = 0;
        #endif

        void keyPressEvent(KeyEvent&) override {}
        void keyReleaseEvent(KeyEvent&) override {}
        void mousePressEvent(MouseEvent&) override {}
        void mouseReleaseEvent(MouseEvent&) override {}
        void mouseMoveEvent(MouseMoveEvent&) override {}
        void mouseScrollEvent(MouseScrollEvent&) override {}

        /* The base doesn't have a virtual destructor because in C++ it's never
           deleted through a pointer to the base. Here we need it, though. */
        virtual ~PublicizedApplication() {}
    };

    struct PyApplication: PublicizedApplication {
        using PublicizedApplication::PublicizedApplication;

        void drawEvent() override {
            #ifdef __clang__
            /* ugh pybind don't tell me I AM THE FIRST ON EARTH to get a
               warning here. Why there's no PYBIND11_OVERLOAD_PURE_NAME_ARG()
               variant *with* arguments and one without? */
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
            #endif
            PYBIND11_OVERLOAD_PURE_NAME(
                void,
                PublicizedApplication,
                "draw_event",
                drawEvent
            );
            #ifdef __clang__
            #pragma GCC diagnostic pop
            #endif
        }

        /* PYBIND11_OVERLOAD_NAME() calls object_api::operator() with implicit
           template param, which is return_value_policy::automatic_reference.
           That later gets changed to return_value_policy::copy in
           type_caster_base::cast() and there's no way to override that  */
        void keyPressEvent(KeyEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "key_press_event",
                keyPressEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }
        void keyReleaseEvent(KeyEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "key_release_event",
                keyReleaseEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }

        void mousePressEvent(MouseEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "mouse_press_event",
                mousePressEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }
        void mouseReleaseEvent(MouseEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "mouse_release_event",
                mouseReleaseEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }
        void mouseMoveEvent(MouseMoveEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "mouse_move_event",
                mouseMoveEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }
        void mouseScrollEvent(MouseScrollEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "mouse_scroll_event",
                mouseScrollEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }
    };

    py::class_<PublicizedApplication, PyApplication> sdl2application{m, "Application", "SDL2 application"};
    sdl2application
        .def_property("swap_interval", &PyApplication::swapInterval,
            [](PublicizedApplication& self, Int interval) {
                self.setSwapInterval(interval);
            }, "Swap interval")
        .def("main_loop_iteration", &PyApplication::mainLoopIteration, "Run one iteration of application main loop");

    PyNonDestructibleClass<PublicizedApplication::InputEvent> inputEvent_{sdl2application, "InputEvent", "Base for input events"};
    py::class_<PublicizedApplication::KeyEvent, PublicizedApplication::InputEvent> keyEvent_{sdl2application, "KeyEvent", "Key event"};
    py::class_<PublicizedApplication::MouseEvent, PublicizedApplication::InputEvent> mouseEvent_{sdl2application, "MouseEvent", "Mouse event"};
    py::class_<PublicizedApplication::MouseMoveEvent, PublicizedApplication::InputEvent> mouseMoveEvent_{sdl2application, "MouseMoveEvent", "Mouse move event"};
    py::class_<PublicizedApplication::MouseScrollEvent, PublicizedApplication::InputEvent> mouseScrollEvent_{sdl2application, "MouseScrollEvent", "Mouse scroll event"};

    std::vector<std::pair<PublicizedApplication::Configuration::WindowFlag, const char*>> windowFlagMap{
        #define DEFINE_FLAG(flag, name) { PublicizedApplication::Configuration::WindowFlag::flag, #name },
        DEFINE_FLAG(Resizable  , RESIZABLE  )
        DEFINE_FLAG(Contextless, CONTEXTLESS)
        DEFINE_FLAG(OpenGL     , OPENGL     )
        #ifndef CORRADE_TARGET_EMSCRIPTEN
        DEFINE_FLAG(Fullscreen , FULLSCREEN  )
        DEFINE_FLAG(Borderless , BORDERLESS  )
        DEFINE_FLAG(Hidden     , HIDDEN      )
        DEFINE_FLAG(Maximized  , MAXIMIZED   )
        DEFINE_FLAG(Minimized  , MINIMIZED   )
        // NOTE: this will be added soon
        // DEFINE_FLAG(Floating   , FLOATING    )
        DEFINE_FLAG(MouseLocked, MOUSE_LOCKED)
        #endif
        // NOTE: Probably it's good to hide this? (otherwise noisy compiler warning)
        // #ifdef MAGNUM_BUILD_DEPRECATED
        // DEFINE_FLAG(AllowHighDpi, ALLOW_HIGH_DPI)
        // #endif
        #if !defined(CORRADE_TARGET_EMSCRIPTEN) && SDL_MAJOR_VERSION*1000 + SDL_MINOR_VERSION*100 + SDL_PATCHLEVEL >= 2006
        DEFINE_FLAG(Vulkan, VULKAN)
        #endif
        #undef DEFINE_FLAG
    };

    std::vector<std::pair<PublicizedApplication::GLConfiguration::Flag, const char*>> glConfigurationFlagMap{
        #define DEFINE_FLAG(flag, name) { PublicizedApplication::GLConfiguration::Flag::flag, #name },
        DEFINE_FLAG(Debug            , DEBUG             )
        DEFINE_FLAG(RobustAccess     , ROBUST_ACCESS     )
        DEFINE_FLAG(ResetIsolation   , RESET_ISOLATION   )
        #ifndef MAGNUM_TARGET_GLES
        DEFINE_FLAG(ForwardCompatible, FORWARD_COMPATIBLE)
        #endif
        #undef DEFINE_FLAG
    };

    application(sdl2application, windowFlagMap, glConfigurationFlagMap);
    inputEvent(inputEvent_);
    keyEvent(keyEvent_);
    mouseEvent(mouseEvent_);
    mouseMoveEvent(mouseMoveEvent_);
    mouseScrollEvent(mouseScrollEvent_);
}

}}

#ifndef MAGNUM_BUILD_STATIC
/* TODO: remove declaration when https://github.com/pybind/pybind11/pull/1863
   is released */
extern "C" PYBIND11_EXPORT PyObject* PyInit_sdl2();
PYBIND11_MODULE(sdl2, m) {
    magnum::platform::sdl2(m);
}
#endif
