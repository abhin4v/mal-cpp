{ lib
, stdenv
, cmake
, pkg-config
, spdlog
, abseil-cpp
, linenoise
}:

stdenv.mkDerivation {
  name = "mal";

  src = lib.sourceByRegex ./. [
    "^src.*"
    "CMakeLists.txt"
  ];

  nativeBuildInputs = [ cmake pkg-config ];
  buildInputs = [ spdlog abseil-cpp linenoise ];
  checkInputs = [ ];

  doCheck = false;
}
