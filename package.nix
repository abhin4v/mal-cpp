{ lib
, stdenv
, cmake
, pkg-config
, spdlog
, abseil-cpp
, readline
}:

stdenv.mkDerivation {
  name = "mal";

  src = lib.sourceByRegex ./. [
    "^src.*"
    "CMakeLists.txt"
  ];

  nativeBuildInputs = [ cmake pkg-config ];
  buildInputs = [ spdlog abseil-cpp readline ];
  checkInputs = [ ];

  doCheck = false;
}
