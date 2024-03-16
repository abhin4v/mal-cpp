{ lib
, stdenv
, cmake
, pkg-config
, immer
, linenoise
}:

stdenv.mkDerivation {
  name = "mal";

  src = lib.sourceByRegex ./. [
    "^src.*"
    "CMakeLists.txt"
  ];

  nativeBuildInputs = [ cmake pkg-config ];
  buildInputs = [ immer linenoise ];
  checkInputs = [ ];

  doCheck = false;
}
