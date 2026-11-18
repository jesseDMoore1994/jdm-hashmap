# a nix flake to build the jdm-hashmap static c library.

{
  description = "A flake to build the jdm-hashmap static c library";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    jdm-alloc.url = "github:jesseDMoore1994/jdm-alloc";
    jdm-array.url = "github:jesseDMoore1994/jdm-array";
    jdm-defs.url = "github:jesseDMoore1994/jdm-defs";
    jdm-log.url = "github:jesseDMoore1994/jdm-log";
  };

  outputs = {
    self,
    nixpkgs,
    jdm-alloc,
    jdm-array,
    jdm-defs,
    jdm-log
  }: let
    system = "x86_64-linux";
    pkgs = import nixpkgs { inherit system; };
    jdm-alloc-lib = jdm-alloc.packages.${system}.jdm-alloc;
    jdm-array-lib = jdm-array.packages.${system}.jdm-array;
    jdm-defs-h = jdm-defs.packages.${system}.jdm-defs;
    jdm-log-lib = jdm-log.packages.${system}.jdm-log;
    deps = [ jdm-alloc-lib jdm-array-lib jdm-defs-h jdm-log-lib ];
    jdm-hashmap = pkgs.stdenv.mkDerivation {
      pname = "jdm-hashmap";
      version = "1.0.0";

      src = ./.;

      buildInputs = [ pkgs.cmake ] ++ deps;

      meta = with pkgs.lib; {
        description = "<description>";
        license = licenses.mit;
        maintainers = with pkgs.maintainers; [ jesseDMoore1994 ];
      };
    };
  in {
    packages.x86_64-linux.jdm-hashmap = with nixpkgs.legacyPackages.x86_64-linux; stdenv.mkDerivation {
      pname = "jdm-hashmap";
      version = "1.0.0";

      src = ./.;

      buildInputs = [ cmake ] ++ deps;

      meta = with lib; {
        description = "<description>";
        license = licenses.mit;
        maintainers = with maintainers; [ jesseDMoore1994 ];
      };

      doCheck = true;
    };

    # setup a devShell for development
    devShells.x86_64-linux.default = with nixpkgs.legacyPackages.x86_64-linux; mkShell {
      buildInputs = [ clang-tools cmake gcc gdb valgrind ] ++ deps;
      shellHook = ''
        echo "Welcome to the jdm-hashmap development shell!"
      '';
    };

    # set up defaultPackage to be the jdm-alloc package
    packages.x86_64-linux.default = self.packages.x86_64-linux.jdm-hashmap;
  };
}
