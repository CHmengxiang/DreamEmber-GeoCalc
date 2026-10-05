// minimal path.h stub — the GUI-side path helpers are unused in the
// headless CAS build; only the include is referenced by global.cc
#ifndef GIAC_PATH_STUB_H
#define GIAC_PATH_STUB_H

namespace giac {
  // std::string helpers used by GUI builds live here upstream.
  // Headless builds (wasm/javagiac) compile without them.
}

#endif // GIAC_PATH_STUB_H
