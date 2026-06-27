{
  description = "EdgeTX - Open source RC radio firmware";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };

        # ---- Git submodules (fetched at evaluation time, needs --impure to work) ----
        submodules = rec {
          accessDenied = builtins.fetchGit {
            url = "https://github.com/raphaelcoeffic/AccessDenied";
            rev = "6b04dfe4d90870f09b8aa41b8ccf952ba47a55e1";
          };
          freeRtos = builtins.fetchGit {
            url = "https://github.com/FreeRTOS/FreeRTOS-Kernel";
            rev = "dbf70559b27d39c1fdb68dfb9a32140b6a6777a0";
          };
          stb = builtins.fetchGit {
            url = "https://github.com/nothings/stb";
            rev = "5c205738c191bcb0abc65c4febfa9bd25ff35234";
          };
          lvgl = builtins.fetchGit {
            url = "https://github.com/EdgeTX/lvgl";
            ref = "release/v8.2";
            rev = "5f129c540ec43a4e5aebff9f77b3688b57a78063";
          };
          uf2 = builtins.fetchGit {
            url = "https://github.com/microsoft/uf2";
            rev = "d03b585ed780ed51bb0d1e6e8cf233aacb408305";
          };
        };

        setupSubmodules = ''
          echo "Setting up git submodules..."
          mkdir -p radio/src/thirdparty
          ln -sfn ${submodules.accessDenied} radio/src/thirdparty/AccessDenied
          ln -sfn ${submodules.freeRtos} radio/src/thirdparty/FreeRTOS
          ln -sfn ${submodules.stb} radio/src/thirdparty/stb
          ln -sfn ${submodules.lvgl} radio/src/thirdparty/lvgl
          ln -sfn ${submodules.uf2} radio/src/thirdparty/uf2
        '';

        pythonDeps = with pkgs.python3Packages; [
          asciitree
          jinja2
          pillow
          libclang
          lz4
          pyelftools
          pydantic
        ];

        nativeDeps = with pkgs; [
          cmake
          git
          python3
        ] ++ pythonDeps;

        # ---- Pre-fetched FetchContent dependencies ----
        # Fixed-output derivations have access to network.
        # Provide sources locally via FETCHCONTENT_SOURCE_DIR_<NAME> so CMake
        # skips the download step entirely.
        fetchContentDeps = {
          imgui = pkgs.fetchFromGitHub {
            owner = "ocornut";
            repo = "imgui";
            rev = "v1.92.6";
            sha256 = "1q20bkal24w0vqlyy4g5612qd8sjz3p161cm326dx3parxi0gxwk";
          };
          googletest = pkgs.fetchgit {
            url = "https://github.com/google/googletest";
            rev = "f8d7d77c06936315286eb55f8de22cd23c188571";
            sha256 = "19c7f248rkg302yrbl5x7irfyi6a9whbpf45wn4bn9fk0625qi5p";
          };
          rsdfu-x86_64 = pkgs.fetchzip {
            url = "https://github.com/EdgeTX/rs-dfu/releases/latest/download/rs_dfu-x86_64-unknown-linux-gnu.tar.gz";
            sha256 = "0dkmij3f9dl0d1myb71nld9ly4wcwg048dsnqhyc025shaw5zfrk";
          };
          rsdfu-aarch64 = pkgs.fetchzip {
            url = "https://github.com/EdgeTX/rs-dfu/releases/latest/download/rs_dfu-aarch64-unknown-linux-gnu.tar.gz";
            sha256 = "0g59fnxw1xh9nha8hd1q9i98plwdi6vbhhq134x4fgsr71zpr5vw";
          };
          miniz = pkgs.fetchFromGitHub {
            owner = "richgel999";
            repo = "miniz";
            rev = "89d7a5f6c3ce8893ea042b0a9d2a2d9975589ac9";
            sha256 = "18f8nwjw1mz6666m4c7bpb4dv323rr6cmqbpzsjmp9yvxigw1m74";
          };
          yaml-cpp = pkgs.fetchFromGitHub {
            owner = "jbeder";
            repo = "yaml-cpp";
            rev = "28f93bdec6387d42332220afa9558060c8016795";
            sha256 = "15rvc90jprgnvqmclm8digyr7rmwyw0m6d0c575hx2x23bhyrpz7";
          };
          maxLibQt = pkgs.fetchFromGitHub {
            owner = "edgetx";
            repo = "maxLibQt";
            rev = "7e433da60d3f2e975d46afc91804a88029cd1b78";
            sha256 = "09ghqn3gbk0q2y7ykdsp36y44dgv6k0fn3wpbixlyb2da16pryfl";
          };
          wamr = pkgs.fetchgit {
            url = "https://github.com/bytecodealliance/wasm-micro-runtime";
            rev = "WAMR-2.4.4";
            sha256 = "13njx6qd5ach18fyiwqajqjry17z85bbbdj9ggc1yxg1m429vnx4";
          };
        };

        # All NATVIE_BUILD=ON targets (simu, companion) configure companion/
        # at cmake time, All FetchContent deps must be overridden for every build.
        # WAMR is staged as a writable copy because it needs write access during
        # configure (version.h generation). ($PWD expands at build time in bash)
        nativeFcFlags = [
          "-DFETCHCONTENT_SOURCE_DIR_IMGUI=${fetchContentDeps.imgui}"
          "-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=${fetchContentDeps.googletest}"
          "-DFETCHCONTENT_SOURCE_DIR_RSDFU=${fetchContentDeps.rsdfu-x86_64}"
          "-DFETCHCONTENT_SOURCE_DIR_MINIZ=${fetchContentDeps.miniz}"
          "-DFETCHCONTENT_SOURCE_DIR_YAML-CPP=${fetchContentDeps.yaml-cpp}"
          "-DFETCHCONTENT_SOURCE_DIR_MAXLIBQT=${fetchContentDeps.maxLibQt}"
          "-DFETCHCONTENT_SOURCE_DIR_WAMR=$PWD/.fc-staging/wamr"
        ];
        nativeFcFlagsAarch64 = [
          "-DFETCHCONTENT_SOURCE_DIR_IMGUI=${fetchContentDeps.imgui}"
          "-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=${fetchContentDeps.googletest}"
          "-DFETCHCONTENT_SOURCE_DIR_RSDFU=${fetchContentDeps.rsdfu-aarch64}"
          "-DFETCHCONTENT_SOURCE_DIR_MINIZ=${fetchContentDeps.miniz}"
          "-DFETCHCONTENT_SOURCE_DIR_YAML-CPP=${fetchContentDeps.yaml-cpp}"
          "-DFETCHCONTENT_SOURCE_DIR_MAXLIBQT=${fetchContentDeps.maxLibQt}"
          "-DFETCHCONTENT_SOURCE_DIR_WAMR=$PWD/.fc-staging/wamr"
        ];

        wamrStageCmd = ''
          mkdir -p $PWD/.fc-staging
          cp -r --no-preserve=mode ${fetchContentDeps.wamr} $PWD/.fc-staging/wamr
          chmod -R +w $PWD/.fc-staging/wamr
        '';

        # ---- Firmware package ----
        mkFirmware =
          {
            pcb,
            pcrev ? "",
            autosource ? "ON",
            autoswitch ? "ON",
            curves ? "ON",
            flightmodes ? "ON",
            gvars ? "ON",
            lua ? "ON",
            luacompiler ? "ON",
            luamixer ? "ON",
            pname ?
          let
            lower = pkgs.lib.strings.toLower;
            clean = s: builtins.replaceStrings [ "+" ] [ "p" ] s;
          in "edgetx-firmware-${clean (lower pcb)}-${clean (lower (if pcrev != "" then pcrev else pcb))}" }:
          let
            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
              "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/arm-none-eabi.cmake"
              "-DEdgeTX_SUPERBUILD=OFF"
              "-DNATIVE_BUILD=OFF"
              "-DUSE_UNSUPPORTED_TOOLCHAIN=ON"
              "-DPCB=${pcb}"
            ] ++ pkgs.lib.optionals (pcrev != "") [
              "-DPCBREV=${pcrev}"
            ] ++ pkgs.lib.optionals (autosource != "") [
              "-DAUTOSOURCE=${autosource}"
            ] ++ pkgs.lib.optionals (autoswitch != "") [
              "-DAUTOSWITCH=${autoswitch}"
            ] ++ pkgs.lib.optionals (curves != "") [
              "-DCURVES=${curves}"
            ] ++ pkgs.lib.optionals (flightmodes != "") [
              "-DFLIGHT_MODES=${flightmodes}"
            ] ++ pkgs.lib.optionals (gvars != "") [
              "-DGVARS=${gvars}"
            ] ++ pkgs.lib.optionals (lua != "") [
              "-DLUA=${lua}"
            ] ++ pkgs.lib.optionals (luacompiler != "") [
              "-DLUA_COMPILER=${luacompiler}"
            ] ++ pkgs.lib.optionals (luamixer != "") [
              "-DLUA_MIXER=${luamixer}"
            ];
          in
          pkgs.stdenv.mkDerivation {
            inherit pname;
            version = "3.0.0";
            src = ./.;

            nativeBuildInputs = nativeDeps ++ [ pkgs.gcc-arm-embedded pkgs.clang ];
            buildInputs = [ pkgs.glibc.dev ];

            NIX_SYSTEM_INCLUDE_DIRS = with pkgs; lib.concatStringsSep ":" [
              "${glibc.dev}/include"
              "${stdenv.cc.cc}/include/c++/${stdenv.cc.cc.version}"
              "${stdenv.cc.cc}/include/c++/${stdenv.cc.cc.version}/${stdenv.targetPlatform.config}"
              "${stdenv.cc.cc}/lib/gcc/${stdenv.targetPlatform.config}/${stdenv.cc.cc.version}/include"
            ];

            postPatch = ''
              ${pkgs.python3}/bin/python3 -c "
              import os, sys
              path = 'radio/util/generate_datacopy.py'
              old = '    if find_clang.builtin_hdr_path:\n        args.append(\"-I\" + find_clang.builtin_hdr_path)'
              new = '    if find_clang.builtin_hdr_path:\n        args.append(\"-I\" + find_clang.builtin_hdr_path)\n    _nix_dirs = os.environ.get(\"NIX_SYSTEM_INCLUDE_DIRS\", \"\")\n    if _nix_dirs:\n        for _d in _nix_dirs.split(\":\"):\n            if os.path.isdir(_d):\n                args.append(\"-idirafter\")\n                args.append(_d)'
              with open(path) as f:
                  c = f.read()
              if old not in c:
                  print('ERROR: pattern not found in ' + path)
                  print(repr(old))
                  sys.exit(1)
              c = c.replace(old, new)
              with open(path, 'w') as f:
                  f.write(c)
              "
            '';

            preConfigure = setupSubmodules;

            configurePhase = ''
              runHook preConfigure
              cmake -B build -S . ${builtins.concatStringsSep " " cmakeFlags} -Wno-dev
              runHook postConfigure
            '';

            buildPhase = ''
              runHook preBuild
              cmake --build build --target firmware -j$(($NIX_BUILD_CORES-1))
              runHook postBuild
            '';

            installPhase = ''
              runHook preInstall
              mkdir -p $out
              cp build/firmware.bin $out/
              cp build/firmware.elf $out/
              runHook postInstall
            '';

            enableParallelBuilding = true;
          };

        # ---- Native SDL simulator ----
        mkSimu =
          { 
            pcb ? "X10",
            pcbrev ? "TX16S",
            buildType ? "Release",
            autosource ? "ON",
            autoswitch ? "ON",
            bootloader ? "OFF",
            curves ? "ON",
            flightmodes ? "ON",
            gvars ? "ON",
            lua ? "ON",
            luacompiler ? "ON",
            luamixer ? "ON",
            extraNativeBuildInputs ? [ ],
            extraBuildInputs ? [ ] ,
          }:
          let
            pkgs = import nixpkgs {
              inherit system;
              overlays = [
                # sdl2-compat's cmake defaults to SDL2COMPAT_X11=ON even when
                # x11Support=false; it only removes libx11 from buildInputs
                # but doesn't pass -DSDL2COMPAT_X11=OFF to cmake, causing
                # cross-compile to fail finding X11 headers.
                (final: prev: {
                  SDL2_wayland = (prev.sdl2-compat.override { x11Support = false; })
                  .overrideAttrs (old: {
                    cmakeFlags = (old.cmakeFlags or []) ++ [
                      "-DSDL2COMPAT_X11=OFF"
                    ];
                  });
                })
              ];
            };

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=${buildType}"
              "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/native.cmake"
              "-DEdgeTX_SUPERBUILD=OFF"
              "-DNATIVE_BUILD=ON"
              "-DPCB=${pcb}"
              "-DCMAKE_TLS_VERIFY=OFF"
              "-DDISABLE_COMPANION=ON"
              "-DAUDIO=ON"
            ] ++ pkgs.lib.optionals (pcbrev != "") [
              "-DPCBREV=${pcbrev}"
            ] ++ pkgs.lib.optionals (autosource != "") [
              "-DAUTOSOURCE=${autosource}"
            ] ++ pkgs.lib.optionals (autoswitch != "") [
              "-DAUTOSWITCH=${autoswitch}"
            ] ++ pkgs.lib.optionals (bootloader != "") [
              "-DBOOTLOADER=${bootloader}"
            ] ++ pkgs.lib.optionals (curves != "") [
              "-DCURVES=${curves}"
            ] ++ pkgs.lib.optionals (flightmodes != "") [
              "-DFLIGHT_MODES=${flightmodes}"
            ] ++ pkgs.lib.optionals (gvars != "") [
              "-DGVARS=${gvars}"
            ] ++ pkgs.lib.optionals (lua != "") [
              "-DLUA=${lua}"
            ] ++ pkgs.lib.optionals (luacompiler != "") [
              "-DLUA_COMPILER=${luacompiler}"
            ] ++ pkgs.lib.optionals (luamixer != "") [
              "-DLUA_MIXER=${luamixer}"
            ] ++ nativeFcFlags;
          in
          pkgs.stdenv.mkDerivation {
            pname = "edgetx-simu";
            version = "3.0.0";
            src = ./.;

            nativeBuildInputs = nativeDeps
              ++ [ pkgs.cacert pkgs.qt6.wrapQtAppsHook ]
              ++ extraNativeBuildInputs;
            buildInputs = with pkgs; [
              SDL2
              openssl
              qt6.qtbase
              qt6.qtmultimedia
              qt6.qtserialport
              qt6.qtsvg
              qt6.qttools
            ] ++ extraBuildInputs;

            SSL_CERT_FILE = "${pkgs.cacert}/etc/ssl/certs/ca-bundle.crt";

            NIX_SYSTEM_INCLUDE_DIRS = with pkgs; lib.concatStringsSep ":" [
              "${stdenv.cc.cc}/include/c++/${stdenv.cc.cc.version}"
              "${stdenv.cc.cc}/include/c++/${stdenv.cc.cc.version}/${stdenv.targetPlatform.config}"
              "${stdenv.cc.cc}/lib/gcc/${stdenv.targetPlatform.config}/${stdenv.cc.cc.version}/include"
              "${glibc.dev}/include"
            ];

            postPatch = ''
              ${pkgs.python3}/bin/python3 -c "
              import os, sys

              # patch generate_datacopy.py
              path = 'radio/util/generate_datacopy.py'
              old = '    if find_clang.builtin_hdr_path:\n        args.append(\"-I\" + find_clang.builtin_hdr_path)'
              new = '    if find_clang.builtin_hdr_path:\n        args.append(\"-I\" + find_clang.builtin_hdr_path)\n    _nix_dirs = os.environ.get(\"NIX_SYSTEM_INCLUDE_DIRS\", \"\")\n    if _nix_dirs:\n        for _d in _nix_dirs.split(\":\"):\n            if os.path.isdir(_d):\n                args.append(\"-idirafter\")\n                args.append(_d)'
              with open(path) as f:
                  c = f.read()
              if old not in c:
                  print('ERROR: pattern not found in ' + path)
                  print(repr(old))
                  sys.exit(1)
              c = c.replace(old, new)
              with open(path, 'w') as f:
                  f.write(c)

              # patch generate_yaml.py (different indentation)
              path = 'radio/util/generate_yaml.py'
              old = 'if find_clang.builtin_hdr_path:\n    args.append(\"-I\" + find_clang.builtin_hdr_path)'
              new = 'if find_clang.builtin_hdr_path:\n    args.append(\"-I\" + find_clang.builtin_hdr_path)\n_nix_dirs = os.environ.get(\"NIX_SYSTEM_INCLUDE_DIRS\", \"\")\nif _nix_dirs:\n    for _d in _nix_dirs.split(\":\"):\n        if os.path.isdir(_d):\n            args.append(\"-idirafter\")\n            args.append(_d)'
              with open(path) as f:
                  c = f.read()
              if old not in c:
                  print('ERROR: pattern not found in ' + path)
                  print(repr(old))
                  sys.exit(1)
              c = c.replace(old, new)
              with open(path, 'w') as f:
                  f.write(c)
              "
            '';

            preConfigure = setupSubmodules + wamrStageCmd;

            configurePhase = ''
              runHook preConfigure
              unset SOURCE_DATE_EPOCH
              cmake -B build -S . ${builtins.concatStringsSep " " cmakeFlags} -Wno-dev
              runHook postConfigure
            '';

            buildPhase = ''
              runHook preBuild
              cmake --build build --target simu -j$(($NIX_BUILD_CORES-1))
              runHook postBuild
            '';

            installPhase = ''
              runHook preInstall
              mkdir -p $out/bin
              cp build/simu $out/bin/
              runHook postInstall
            '';

            enableParallelBuilding = true;

            meta.mainProgram = "simu";
          };
        # ---- Companion + Standalone Simulator (Qt6) ----
        mkCompanion =
          { extraNativeBuildInputs ? [ ], extraBuildInputs ? [ ] }:
          let
            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
              "-DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/native.cmake"
              "-DEdgeTX_SUPERBUILD=OFF"
              "-DNATIVE_BUILD=ON"
              "-DCMAKE_TLS_VERIFY=OFF"
            ] ++ nativeFcFlags;
          in
          pkgs.stdenv.mkDerivation {
            pname = "edgetx-companion";
            version = "3.0.0";
            src = ./.;

            nativeBuildInputs = nativeDeps
              ++ [ pkgs.cacert pkgs.qt6.wrapQtAppsHook ]
              ++ extraNativeBuildInputs;
            buildInputs = with pkgs; [
              SDL2
              openssl
              qt6.qtbase
              qt6.qtmultimedia
              qt6.qtserialport
              qt6.qtsvg
              qt6.qttools
            ] ++ extraBuildInputs;

            SSL_CERT_FILE = "${pkgs.cacert}/etc/ssl/certs/ca-bundle.crt";

            NIX_SYSTEM_INCLUDE_DIRS = with pkgs; lib.concatStringsSep ":" [
              "${glibc.dev}/include"
              "${stdenv.cc.cc}/include/c++/${stdenv.cc.cc.version}"
              "${stdenv.cc.cc}/include/c++/${stdenv.cc.cc.version}/${stdenv.targetPlatform.config}"
              "${stdenv.cc.cc}/lib/gcc/${stdenv.targetPlatform.config}/${stdenv.cc.cc.version}/include"
            ];

            postPatch = ''
              ${pkgs.python3}/bin/python3 -c "
              import os, sys
              path = 'radio/util/generate_datacopy.py'
              old = '    if find_clang.builtin_hdr_path:\n        args.append(\"-I\" + find_clang.builtin_hdr_path)'
              new = '    if find_clang.builtin_hdr_path:\n        args.append(\"-I\" + find_clang.builtin_hdr_path)\n    _nix_dirs = os.environ.get(\"NIX_SYSTEM_INCLUDE_DIRS\", \"\")\n    if _nix_dirs:\n        for _d in _nix_dirs.split(\":\"):\n            if os.path.isdir(_d):\n                args.append(\"-idirafter\")\n                args.append(_d)'
              with open(path) as f:
                  c = f.read()
              if old not in c:
                  print('ERROR: pattern not found in ' + path)
                  print(repr(old))
                  sys.exit(1)
              c = c.replace(old, new)
              with open(path, 'w') as f:
                  f.write(c)
              "
            '';

            preConfigure = setupSubmodules + wamrStageCmd;

            configurePhase = ''
              runHook preConfigure
              cmake -B build -S . ${builtins.concatStringsSep " " cmakeFlags} -Wno-dev
              runHook postConfigure
            '';

            buildPhase = ''
              runHook preBuild
              cmake --build build --target companion -j$(($NIX_BUILD_CORES-1))
              cmake --build build --target simulator -j$(($NIX_BUILD_CORES-1))
              runHook postBuild
            '';

            installPhase = ''
              runHook preInstall
              mkdir -p $out/bin
              cp build/companion30 $out/bin/companion
              if [ -f build/simulator30 ]; then
                cp build/simulator30 $out/bin/simulator
              elif [ -f build/simu ]; then
                cp build/simu $out/bin/simulator
              fi
              runHook postInstall
            '';

            enableParallelBuilding = true;

            meta.mainProgram = "companion";
          };

      in
      {
        packages = {

          edgetx-firmware-tx16s = mkFirmware {
            pcb = "X10";
            pcrev = "TX16S";
          };

          edgetx-simu = mkSimu { };

          edgetx-simu-pi = mkSimu {
            pcb = "PI";
            pcbrev = "";
          };

          edgetx-companion = mkCompanion { };

        }
        # Cross-compiled simu for Raspberry Pi 5 (aarch64) — only from x86_64
        // (if system == "x86_64-linux" then
          let
            pkgsArm = import nixpkgs {
              inherit system;
              crossSystem = nixpkgs.lib.systems.examples.aarch64-multiplatform;
              overlays = [
                # sdl2-compat's cmake defaults to SDL2COMPAT_X11=ON even when
                # x11Support=false; it only removes libx11 from buildInputs
                # but doesn't pass -DSDL2COMPAT_X11=OFF to cmake, causing
                # cross-compile to fail finding X11 headers.
                (final: prev: {
                  sdl2-compat = (prev.sdl2-compat.override { x11Support = false; }).overrideAttrs (old: {
                    cmakeFlags = (old.cmakeFlags or []) ++ [
                      #"-DSDL2COMPAT_X11=OFF"
                    ];
                  });
                })
              ];
            };

            aarch64Toolchain = pkgs.writeText "toolchain-aarch64.cmake" ''
              set(CMAKE_SYSTEM_NAME Linux)
              set(CMAKE_SYSTEM_PROCESSOR aarch64)
              set(CMAKE_CXX_STANDARD 17)
              set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
              set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
              set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
            '';

            pythonDepsArm = with pkgsArm.pkgsBuildBuild.python3Packages; [
              asciitree
              jinja2
              pillow
              libclang
              lz4
              pyelftools
              pydantic
            ];

          in
          {
            edgetx-simu-aarch64 = pkgsArm.stdenv.mkDerivation {
              pname = "edgetx-simu-aarch64";
              version = "3.0.0";
              src = ./.;

              nativeBuildInputs = [
                pkgs.cmake
                pkgs.git
                pkgs.python3
                pkgs.cacert
              ] ++ pythonDepsArm;

              buildInputs = [
                pkgsArm.SDL2
                pkgsArm.openssl
              ];

              SSL_CERT_FILE = "${pkgs.cacert}/etc/ssl/certs/ca-bundle.crt";

              # generate_datacopy.py runs on build host (x86_64), so use build-host headers
            NIX_SYSTEM_INCLUDE_DIRS = with pkgs; lib.concatStringsSep ":" [
              "${glibc.dev}/include"
              "${stdenv.cc.cc}/include/c++/${stdenv.cc.cc.version}"
              "${stdenv.cc.cc}/include/c++/${stdenv.cc.cc.version}/${stdenv.targetPlatform.config}"
              "${stdenv.cc.cc}/lib/gcc/${stdenv.targetPlatform.config}/${stdenv.cc.cc.version}/include"
            ];

            postPatch = ''
              ${pkgs.python3}/bin/python3 -c "
              import os, sys

              # patch generate_datacopy.py
              path = 'radio/util/generate_datacopy.py'
              old = '    if find_clang.builtin_hdr_path:\n        args.append(\"-I\" + find_clang.builtin_hdr_path)'
              new = '    if find_clang.builtin_hdr_path:\n        args.append(\"-I\" + find_clang.builtin_hdr_path)\n    _nix_dirs = os.environ.get(\"NIX_SYSTEM_INCLUDE_DIRS\", \"\")\n    if _nix_dirs:\n        for _d in _nix_dirs.split(\":\"):\n            if os.path.isdir(_d):\n                args.append(\"-idirafter\")\n                args.append(_d)'
              with open(path) as f:
                  c = f.read()
              if old not in c:
                  print('ERROR: pattern not found in ' + path)
                  print(repr(old))
                  sys.exit(1)
              c = c.replace(old, new)
              with open(path, 'w') as f:
                  f.write(c)

              # patch generate_yaml.py (different indentation)
              path = 'radio/util/generate_yaml.py'
              old = 'if find_clang.builtin_hdr_path:\n    args.append(\"-I\" + find_clang.builtin_hdr_path)'
              new = 'if find_clang.builtin_hdr_path:\n    args.append(\"-I\" + find_clang.builtin_hdr_path)\n_nix_dirs = os.environ.get(\"NIX_SYSTEM_INCLUDE_DIRS\", \"\")\nif _nix_dirs:\n    for _d in _nix_dirs.split(\":\"):\n        if os.path.isdir(_d):\n            args.append(\"-idirafter\")\n            args.append(_d)'
              with open(path) as f:
                  c = f.read()
              if old not in c:
                  print('ERROR: pattern not found in ' + path)
                  print(repr(old))
                  sys.exit(1)
              c = c.replace(old, new)
              with open(path, 'w') as f:
                  f.write(c)
              "
            '';

              preConfigure = setupSubmodules + wamrStageCmd;

              configurePhase = ''
                runHook preConfigure
                cmake -B build -S . \
                  -DCMAKE_BUILD_TYPE=Release \
                  -DCMAKE_TOOLCHAIN_FILE=${aarch64Toolchain} \
                  -DEdgeTX_SUPERBUILD=OFF \
                  -DNATIVE_BUILD=ON \
                  -DPCB=X10 -DPCBREV=TX16S \
                  -DCMAKE_TLS_VERIFY=OFF \
                  -DDISABLE_COMPANION=ON \
                  ${builtins.concatStringsSep " \\\n                  " nativeFcFlagsAarch64} \
                  -Wno-dev
                runHook postConfigure
              '';

              buildPhase = ''
                runHook preBuild
                cmake --build build --target simu -j$(($NIX_BUILD_CORES-1))
                runHook postBuild
              '';

              installPhase = ''
                runHook preInstall
                mkdir -p $out/bin
                cp build/simu $out/bin/
                runHook postInstall
              '';

              enableParallelBuilding = true;

              meta.mainProgram = "simu";
            };
          }
        else { });

        apps = {
          # Runnable apps
          simu = flake-utils.lib.mkApp {
            drv = self.packages.${system}.edgetx-simu;
            name = "simu";
          };
          companion = flake-utils.lib.mkApp {
            drv = self.packages.${system}.edgetx-companion;
            name = "companion";
          };

          # Build apps — each creates a named --out-link so builds don't overwrite each other
          build-firmware = {
            type = "app";
            program = let
              script = pkgs.writeShellScript "build-firmware" ''
                exec nix build "path:${toString ./.}#edgetx-firmware-tx16s" \
                  --out-link firmware \
                  --impure "$@"
              '';
            in "${script}";
          };
          build-simu = {
            type = "app";
            program = let
              script = pkgs.writeShellScript "build-simu" ''
                exec nix build "path:${toString ./.}#edgetx-simu" \
                  --out-link simu \
                  --impure "$@"
              '';
            in "${script}";
          };
          build-companion = {
            type = "app";
            program = let
              script = pkgs.writeShellScript "build-companion" ''
                exec nix build "path:${toString ./.}#edgetx-companion" \
                  --out-link edgetx-companion \
                  --impure "$@"
              '';
            in "${script}";
          };
          build-simu-aarch64 = {
            type = "app";
            program = let
              script = pkgs.writeShellScript "build-simu-aarch64" ''
                exec nix build "path:${toString ./.}#edgetx-simu-aarch64" \
                  --out-link simu-aarch64 \
                  --impure "$@"
              '';
            in "${script}";
          };
          build-simu-pi = {
            type = "app";
            program = let
              script = pkgs.writeShellScript "build-simu-pi" ''
                exec nix build "path:${toString ./.}#edgetx-simu-pi" \
                  --out-link simu-pi \
                  --impure "$@"
              '';
            in "${script}";
          };
        };

        devShells.default = pkgs.mkShell {
          name = "edgetx-dev";

          inputsFrom = [
            (mkSimu { extraNativeBuildInputs = [ pkgs.gcc-arm-embedded ]; })
            (mkCompanion { })
          ];

          packages = [ pkgs.gcc-arm-embedded pkgs.nodejs pkgs.lv_font_conv ];

          # A simple banner with all options for the shell
          shellHook = ''
            echo "EdgeTX development shell"
            echo "Build commands (aliases):"
            echo "  nix run .#build-firmware   → builds firmware, creates firmware/ symlink"
            echo "  nix run .#build-simu       → builds SDL simu, creates simu/ symlink"
            echo "  nix run .#build-companion  → builds companion, creates edgetx-companion/ symlink"
            echo "  nix run .#build-simu-aarch64 → cross-compiles simu for ARM, creates simu-aarch64/ symlink"
            echo "  nix run .#build-simu-pi      → builds SDL simu (Pi target), creates simu-pi/ symlink"
            echo "NOTE: --impure still needed (builtins.fetchGit for submodules)"
            echo "      FetchContent deps are pre-fetched — no --option sandbox false needed!"
            echo ""
          '';
        };
      });
}
