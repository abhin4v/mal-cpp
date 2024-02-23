{ lib
, stdenv
, cmake
, pkg-config
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
  buildInputs = [ abseil-cpp linenoise ];
  checkInputs = [ ];

  doCheck = false;
}
