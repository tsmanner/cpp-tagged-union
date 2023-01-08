let pkgs = import <nixpkgs> {}; in
{ cc ? "clang_14",
}: pkgs.runCommand "dummy" { buildInputs = [
  pkgs.ninja pkgs.git pkgs.cmake pkgs.openssh pkgs.cacert
  # Configurable compiler selection
  pkgs."${cc}"
]; } ""
