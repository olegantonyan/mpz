{ libfault ? { outPath = ./.; revCount = 0; shortRev = "abcdef"; rev = "HEAD"; }
, officialRelease ? false
, coverityUpload ? false
}:

let
  nixcfg = { allowUnfree = true; };
  pkgs   = import <nixpkgs> { config=nixcfg; };

  systems = [ "i686-linux" "x86_64-linux"
             #"x86_64-darwin"
            ];

  version = builtins.readFile ./VERSION +
    (pkgs.lib.optionalString (!officialRelease)
      "-r${toString libfault.revCount}-g${libfault.shortRev}");

  ifNotCoverity = x: if coverityUpload == true  then {} else x;
  ifCoverity    = x: if coverityUpload == false then {} else x;

  jobs = rec {
    ## -- Tarballs -------------------------------------------------------------
    tarball = pkgs.releaseTools.sourceTarball {
      name = "libfault-tarball";
      src  = libfault;
      inherit version;
      buildInputs = with pkgs; [ git xz perl ];
      meta.maintainers = [ "aseipp@pobox.com" ];

      distPhase = ''
        relname=libfault-${version}
        mkdir ../$relname
        cp -prd . ../$relname
        rm -rf ../$relname/.git ../$relname/svn-revision
        echo -n ${version} > ../$relname/VERSION

        mkdir $out/tarballs
        tar cvfJ $out/tarballs/$relname.tar.xz -C .. $relname
      '';
    };

    ## -- Build ----------------------------------------------------------------
    build = ifNotCoverity (pkgs.lib.genAttrs systems (system:
      with import <nixpkgs> { inherit system; config=nixcfg; };

      releaseTools.nixBuild {
        name = "libfault";
        inherit version;
        src  = tarball;
        buildInputs = with pkgs; [ perl ];
        meta.maintainers = [ "aseipp@pobox.com" ];

        enableParallelBuilding = true;
        doCheck = false;

        installPhase = "make install PREFIX=$out";
      }
    ));

    ## -- Tests ----------------------------------------------------------------
    tests = ifNotCoverity (pkgs.lib.genAttrs systems (system:
      with import <nixpkgs> { inherit system; config=nixcfg; };

      releaseTools.nixBuild {
        name = "libfault-test";
        inherit version;
        src  = tarball;
        buildInputs = with pkgs; [ perl ];
        meta.maintainers = [ "aseipp@pobox.com" ];

        enableParallelBuilding = true;
        doCheck      = true;
        checkFlags   = [ "DEBUG=1" ];
        dontInstall  = true;
      }
    ));

    ## -- Coverity -------------------------------------------------------------
    coverity = ifCoverity (
      with import <nixpkgs> { system = "x86_64-linux"; config=nixcfg; };

      releaseTools.coverityAnalysis {
        name = "libfault-coverity";
        inherit version;
        src  = tarball;
        buildInputs = with pkgs; [ perl ];
        meta.maintainers = [ "aseipp@pobox.com" ];

        buildPhase = "make all check DEBUG=1";
        doCheck    = false;
      }
    );

    ## -- Clang analyzer -------------------------------------------------------
    clang-analyzer = ifNotCoverity (
      with import <nixpkgs> { system = "x86_64-linux"; config=nixcfg; };

      releaseTools.clangAnalysis {
        name = "libfault-clang-analyzer";
        inherit version;
        src  = tarball;
        buildInputs = with pkgs; [ perl ];
        meta.maintainers = [ "aseipp@pobox.com" ];

        buildPhase = "make all check DEBUG=1";
        doCheck    = false;
      }
    );

    ## -- Coverage -------------------------------------------------------------
    coverage = ifNotCoverity (
      with import <nixpkgs> { system = "x86_64-linux"; config=nixcfg; };

      releaseTools.coverageAnalysis {
        name = "libfault-coverage";
        inherit version;
        src  = tarball;
        buildInputs = with pkgs; [ perl ];
        meta.maintainers = [ "aseipp@pobox.com" ];

        enableParallelBuilding = true;
        makeFlags = [ "DEBUG=1" ];
      }
    );

    ## -- Debian build ---------------------------------------------------------

    #deb_debian7i386   = ifNotCoverity
    #  (makeDeb_i686 (diskImageFuns: diskImageFuns.debian7i386) 60);
    #deb_debian7x86_64 = ifNotCoverity
    #  (makeDeb_x86_64 (diskImageFunsFun: diskImageFunsFun.debian7x86_64) 60);

    ## -- Ubuntu builds --------------------------------------------------------

    #deb_ubuntu1004i386   = ifNotCoverity
    #  (makeDeb_i686 (diskImageFuns: diskImageFuns.ubuntu1004i386) 50);
    #deb_ubuntu1004x86_64 = ifNotCoverity
    #  (makeDeb_x86_64 (diskImageFuns: diskImageFuns.ubuntu1004x86_64) 50);
    #deb_ubuntu1204i386   = ifNotCoverity
    #  (makeDeb_i686 (diskImageFuns: diskImageFuns.ubuntu1204i386) 60);
    #deb_ubuntu1204x86_64 = ifNotCoverity
    #  (makeDeb_x86_64 (diskImageFuns: diskImageFuns.ubuntu1204x86_64) 60);
    #deb_ubuntu1404i386   = ifNotCoverity
    #  (makeDeb_i686 (diskImageFuns: diskImageFuns.ubuntu1404i386) 60);
    #deb_ubuntu1404x86_64 = ifNotCoverity
    #  (makeDeb_x86_64 (diskImageFuns: diskImageFuns.ubuntu1404x86_64) 60);

    ## -- Fedora builds --------------------------------------------------------

    #rpm_fedora20i386   = ifNotCoverity
    #  (makeRPM_i686 (diskImageFuns: diskImageFuns.fedora20i386) 70);
    #rpm_fedora20x86_64 = ifNotCoverity
    #  (makeRPM_x86_64 (diskImageFunsFun: diskImageFunsFun.fedora20x86_64) 70);

    ## -- Release build --------------------------------------------------------
    release = ifNotCoverity (pkgs.releaseTools.aggregate
      { name = "libfault-${version}";
        constituents =
          [ tarball
            build.i686-linux build.x86_64-linux # build.x86_64-darwin
            tests.i686-linux tests.x86_64-linux # tests.x864_64-darwin
           #deb_debian7i386    deb_debian7x86_64
           #deb_ubuntu1004i386 deb_ubuntu1004x86_64
           #deb_ubuntu1204i386 deb_ubuntu1204x86_64
           #deb_ubuntu1404i386 deb_ubuntu1404x86_64
           #rpm_fedora20i386   rpm_fedora20x86_64
            coverage
          ];
        meta.description = "Release-critical builds";
        meta.maintainers = [ "aseipp@pobox.com" ];
      }
    );
  };

  ## -- RPM/Deb utilities ----------------------------------------------------

  makeDeb_i686 = makeDeb "i686-linux";
  makeDeb_x86_64 = makeDeb "x86_64-linux";

  makeDeb =
    system: diskImageFun: prio:

    with import <nixpkgs> { inherit system; };

    releaseTools.debBuild {
      name = "libfault-deb";
      src = jobs.tarball;
      diskImage = (diskImageFun vmTools.diskImageFuns) {};
      memSize = 1024;
      meta.schedulingPriority = prio;
      debMaintainer = "Austin Seipp <aseipp@pobox.com>";
      doInstallCheck = false;
      preInstall = "export INSTALLPREFIX=/usr";
    };


  makeRPM_i686   = makeRPM "i686-linux";
  makeRPM_x86_64 = makeRPM "x86_64-linux";

  makeRPM =
    system: diskImageFun: prio:

    with import <nixpkgs> { inherit system; };

    releaseTools.rpmBuild rec {
      name = "libfault-rpm";
      src = jobs.tarball;
      diskImage = (diskImageFun vmTools.diskImageFuns) {};
      memSize = 1024;
      meta.schedulingPriority = prio;
      preInstall = "export INSTALLPREFIX=/usr";
    };

in jobs
