let
  pkgs   = import <nixpkgs> {};
  stdenv = pkgs.stdenv;

  env = with pkgs; rec {
    libfaultEnv = stdenv.mkDerivation rec {
      name = "libfault-env";
      src  = ./.;
      buildInputs = [ git xz clang perl gdb rr ];
    };
  };
in env
