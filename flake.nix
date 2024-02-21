{
  description = "Make-A-Lisp in CPP";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    mini-compile-commands = {
      url = "github:danielbarter/mini_compile_commands";
      flake = false;
    };
  };

  outputs = inputs@{ flake-parts, mini-compile-commands, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [
        # To import a flake module
        # 1. Add foo to inputs
        # 2. Add foo as a parameter to the outputs function
        # 3. Add here: foo.flakeModule

      ];
      systems = [ "x86_64-linux" "aarch64-linux" "aarch64-darwin" "x86_64-darwin" ];
      perSystem = { config, self', inputs', pkgs, system, ... }: {
        # Per-system attributes can be defined here. The self' and inputs'
        # module parameters provide easy access to attributes of the same
        # system.

        packages.default = pkgs.callPackage ./package.nix { stdenv = pkgs.clangStdenv; };

        devShells.default = let
          mcc-env = (pkgs.callPackage mini-compile-commands { }).wrap pkgs.clangStdenv;
          mccShell = pkgs.mkShell.override { stdenv = mcc-env; };
        in mccShell {
          nativeBuildInputs = self'.packages.default.nativeBuildInputs;
          buildInputs = self'.packages.default.buildInputs;
          packages = with pkgs; [ just clang-tools cppcheck flawfinder python3Minimal ];
        };
      };
      flake = {
        # The usual flake attributes can be defined here, including system-
        # agnostic ones like nixosModule and system-enumerating ones, although
        # those are more easily expressed in perSystem.

      };
    };
}
