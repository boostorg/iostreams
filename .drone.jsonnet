# Copyright 2022 Peter Dimov
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

local library = "iostreams";

local triggers =
{
    branch: [ "master", "develop", "feature/*", "fix/*" ]
};

local ubsan = { UBSAN: '1', UBSAN_OPTIONS: 'print_stacktrace=1' };
local asan = { ASAN: '1' };

// General Linux and macOS compiler coverage is provided by GitHub Actions.
local linux_pipeline(name, image, environment, packages, sources = [ "ppa:ubuntu-toolchain-r/test" ], script = "drone", trigger = triggers) =
local compression_packages = [ "zlib1g-dev", "libbz2-dev", "liblzma-dev", "libzstd-dev" ];
local multilib = std.objectHas(environment, "ADDRMD") && std.member(std.split(environment.ADDRMD, ","), "32");
{
    name: name,
    kind: "pipeline",
    type: "docker",
    trigger: trigger,
    platform:
    {
        os: "linux",
        arch: "amd64"
    },
    steps:
    [
        {
            name: "everything",
            image: image,
            environment: environment,
            commands:
            [
                'set -e',
                'echo $DRONE_STAGE_MACHINE',
                'uname -a',
                'curl -sSL --retry 5 https://apt.llvm.org/llvm-snapshot.gpg.key | sudo gpg --dearmor -o /etc/apt/trusted.gpg.d/llvm-snapshot.gpg',
            ] +
            [ ('apt-add-repository -y --no-update "' + source + '"') for source in sources ] +
            (if multilib then [ 'dpkg --add-architecture i386' ] else []) +
            [
                'apt-get update',
                'apt-get -y install ' + (if packages != "" then packages + ' ' else '') + std.join(' ', compression_packages +
                    (if multilib then [ package + ':i386' for package in compression_packages ] else [])),
            ] +
            [
                'export LIBRARY=' + library,
                'bash ./.drone/' + script + '.sh',
            ]
        }
    ]
};

local windows_pipeline(name, image, environment, arch = "amd64") =
{
    name: name,
    kind: "pipeline",
    type: "docker",
    trigger: triggers,
    platform:
    {
        os: "windows",
        arch: arch
    },
    "steps":
    [
        {
            name: "everything",
            image: image,
            environment: environment,
            commands:
            [
                'echo $env:DRONE_STAGE_MACHINE',
                // Preserve the path separator through Jsonnet and Drone substitution.
                'cmd /C .drone\\\\drone.bat ' + library,
            ]
        }
    ]
};

[
    linux_pipeline(
        "Linux 24.04 GCC 16 32/64 UBSAN C++11-14",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-16', CXXSTD: '11,14', ADDRMD: '32,64' } + ubsan,
        "g++-16-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 16 32/64 UBSAN C++17-20",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-16', CXXSTD: '17,20', ADDRMD: '32,64' } + ubsan,
        "g++-16-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 16 32/64 UBSAN C++23",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-16', CXXSTD: '23', ADDRMD: '32,64' } + ubsan,
        "g++-16-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 16 32 ASAN",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-16', CXXSTD: '11,14,17,20,23', ADDRMD: '32' } + asan,
        "g++-16-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 GCC 16 64 ASAN",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-16', CXXSTD: '11,14,17,20,23', ADDRMD: '64' } + asan,
        "g++-16-multilib",
    ),

    linux_pipeline(
        "Linux 24.04 Clang 23 32 UBSAN",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'clang', COMPILER: 'clang++-23', CXXSTD: '11,14,17,20,23', ADDRMD: '32' } + ubsan,
        "clang-23 libclang-rt-23-dev g++-16-multilib",
        [ "ppa:ubuntu-toolchain-r/test", "deb [arch=amd64] https://apt.llvm.org/noble/ llvm-toolchain-noble-23 main" ],
    ),

    // Keep each UBSan architecture within Drone's per-job timeout.
    linux_pipeline(
        "Linux 24.04 Clang 23 64 UBSAN",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'clang', COMPILER: 'clang++-23', CXXSTD: '11,14,17,20,23', ADDRMD: '64' } + ubsan,
        "clang-23 libclang-rt-23-dev g++-16-multilib",
        [ "ppa:ubuntu-toolchain-r/test", "deb [arch=amd64] https://apt.llvm.org/noble/ llvm-toolchain-noble-23 main" ],
    ),

    linux_pipeline(
        "Linux 24.04 Clang 23 32 ASAN",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'clang', COMPILER: 'clang++-23', CXXSTD: '11,14,17,20,23', ADDRMD: '32' } + asan,
        "clang-23 libclang-rt-23-dev g++-16-multilib",
        [ "ppa:ubuntu-toolchain-r/test", "deb [arch=amd64] https://apt.llvm.org/noble/ llvm-toolchain-noble-23 main" ],
    ),

    linux_pipeline(
        "Linux 24.04 Clang 23 64 ASAN",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'clang', COMPILER: 'clang++-23', CXXSTD: '11,14,17,20,23', ADDRMD: '64' } + asan,
        "clang-23 libclang-rt-23-dev g++-16-multilib",
        [ "ppa:ubuntu-toolchain-r/test", "deb [arch=amd64] https://apt.llvm.org/noble/ llvm-toolchain-noble-23 main" ],
    ),

    linux_pipeline(
        "Linux Codecov coverage",
        "cppalliance/droneubuntu2404:1",
        { TOOLSET: 'gcc', COMPILER: 'g++-16', CXXSTD: '23', GCOV: 'gcov-16' },
        "g++-16 python3-venv",
        script = "codecov",
    ) + {
        steps+: [
            {
                name: "upload coverage",
                image: "cppalliance/droneubuntu2404:1",
                environment: {
                    CODECOV_TOKEN: { from_secret: "codecov_token" },
                },
                commands: [ 'bash ./.drone/codecov.sh upload' ],
            },
        ],
    },

    linux_pipeline(
        "Linux Coverity Scan",
        "cppalliance/droneubuntu2404:1",
        {
            TOOLSET: 'gcc', COMPILER: 'g++', CXXSTD: '17',
            COVERITY_SCAN_TOKEN: { from_secret: "coverity_scan_token" },
            COVERITY_SCAN_NOTIFICATION_EMAIL: { from_secret: "coverity_scan_email" },
        },
        // Use the distribution compiler for Coverity capture compatibility.
        "g++",
        sources = [],
        script = "coverity",
        trigger = { branch: [ "master", "develop" ], event: [ "push" ] },
    ),

    windows_pipeline(
        "Windows VS2015 msvc-14.0",
        "cppalliance/dronevs2015",
        { TOOLSET: 'msvc-14.0', CXXSTD: '14,latest', B2_DONT_EMBED_MANIFEST: '1' },
    ),

    windows_pipeline(
        "Windows VS2017 msvc-14.1",
        "cppalliance/dronevs2017",
        { TOOLSET: 'msvc-14.1', CXXSTD: '14,17,latest' },
    ),

    windows_pipeline(
        "Windows VS2019 msvc-14.2",
        "cppalliance/dronevs2019",
        { TOOLSET: 'msvc-14.2', CXXSTD: '14,17,20,latest' },
    ),

    windows_pipeline(
        "Windows VS2022 msvc-14.3",
        "cppalliance/dronevs2022:1",
        { TOOLSET: 'msvc-14.3', CXXSTD: '14,17,20,latest' },
    ),
]
