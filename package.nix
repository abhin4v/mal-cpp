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

  fixupPhase = ''
    LIBLINENOISE_PATH=`pkg-config --libs-only-L linenoise | cut -c 3-`;
    for prog in $(find $out -name "step*"); do
      echo "Fixing $prog";
      install_name_tool -change liblinenoise.so "$LIBLINENOISE_PATH"/liblinenoise.so "$prog";
    done
  '';

  doCheck = false;
}
