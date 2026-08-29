{
  description = "Volume keys control daemon for Xfce using pulseaudio";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = pkgs.stdenv.mkDerivation {
            pname = "xfce4-volumed-pulse";
            version = "0.3.0-dev";

            src = self;

            nativeBuildInputs = with pkgs; [
              meson
              ninja
              pkg-config
              gettext
              git
              wrapGAppsHook3
            ];

            buildInputs = with pkgs; [
              glib
              gtk3
              keybinder3
              libnotify
              libpulseaudio
              xfce.xfconf
            ];

            meta = with pkgs.lib; {
              description = "Volume keys control daemon for Xfce using pulseaudio";
              homepage = "https://gitlab.xfce.org/apps/xfce4-volumed-pulse";
              license = licenses.gpl3Plus;
              platforms = platforms.linux;
              mainProgram = "xfce4-volumed-pulse";
            };
          };
        }
      );

      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = pkgs.mkShell {
            inputsFrom = [ self.packages.${system}.default ];
            packages = with pkgs; [
              meson
              ninja
              pkg-config
              gettext
            ];
          };
        }
      );
    };
}
