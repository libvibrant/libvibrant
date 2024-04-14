{
  lib,
  stdenv,
  cmake,
  libX11,
  libXrandr,
  linuxPackages,
}:

stdenv.mkDerivation {
  pname = "libvibrant";
  version = "2100c09";

  src = lib.fileset.toSource {
    root = ../.;
    fileset = lib.fileset.unions (
      map (fileName: ../${fileName}) [
        "cli"
        "cmake"
        "include"
        "src"
        "CMakeLists.txt"
      ]
    );
  };

  buildInputs = [
    libX11
    libXrandr
    linuxPackages.nvidia_x11.settings.libXNVCtrl
  ];
  nativeBuildInputs = [ cmake ];

  meta = with lib; {
    description = "A simple library to adjust color saturation of X11 outputs";
    homepage = "https://github.com/libvibrant/libvibrant";
    license = licenses.gpl3;
    platforms = platforms.linux;
    maintainers = with maintainers; [ Scrumplex ];
    mainProgram = "vibrant-cli";
  };
}
