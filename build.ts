import {Builder} from "@sweetacid/mei";

const packages = [
    "fmt",
];

const cxxFlags = [
    "-Werror=return-type",
];

export default async function (builder: Builder) {
    const ryuko = builder
        .addLibrary("ryuko")
        .include("src")
        .addPackages(...packages)
        .setCXXFlags(...cxxFlags)
        .setCXXStandard("20")
        .link("stdc++")
        .addDirectory("src/ryuko")
        .setPCXXHeader("src/ryuko/pch.hpp");

    await ryuko.build();

    const cli = builder
        .addExecutable("cli")
        .dependOn(ryuko)
        .include("cli", "src")
        .setCXXFlags(...cxxFlags)
        .setCXXStandard("20")
        .link("stdc++")
        .addDirectory("cli");

    await cli.build();
}
