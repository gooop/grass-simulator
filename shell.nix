{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  nativeBuildInputs = [ pkgs.cmake ];
  buildInputs = [
    pkgs.gcc
    pkgs.xorg.libX11
    pkgs.glib
    pkgs.pkgconfig
    pkgs.bear
    pkgs.glfw
    pkgs.glew
    pkgs.glew.dev
    pkgs.glm
    pkgs.freetype
    pkgs.gnumake
    pkgs.stb
    pkgs.clang
    pkgs.renderdoc
    pkgs.gdb

    # keep this line if you use bash
    pkgs.bashInteractive
  ];

  shellHook = ''
    export INCLUDE_DIRS="${pkgs.glfw}/include;${pkgs.stb}/include/stb/;${pkgs.glew.dev}/include/"
    '';
}
