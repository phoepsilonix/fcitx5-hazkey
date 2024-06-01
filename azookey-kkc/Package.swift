// swift-tools-version: 5.10
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
  name: "azookey-kkc",
  products: [
    // Products define the executables and libraries a package produces, making them visible to other packages.
    .library(
      name: "azookey-kkc",
      type: .dynamic,
      targets: ["azookey-kkc"])
  ],
  dependencies: [
    .package(
      url: "https://github.com/ensan-hcl/AzooKeyKanaKanjiConverter",
      .revision("cce5a634a6e59d5440d3a7149af3ca41f6f1efb3"))
  ],
  targets: [
    // Targets are the basic building blocks of a package, defining a module or a test suite.
    // Targets can depend on other targets in this package and products from dependencies.
    .target(
      name: "azookey-kkc",
      dependencies: [
        .product(
          name: "KanaKanjiConverterModuleWithDefaultDictionary",
          package: "AzooKeyKanaKanjiConverter"),
        .product(
          name: "SwiftUtils",
          package: "AzooKeyKanaKanjiConverter"),
      ]
    ),
    .testTarget(
      name: "azookey-kkc-Tests",
      dependencies: [
        "azookey-kkc"
        //.product(name: "KanaKanjiConverterModuleWithDefaultDictionary", package: "AzooKeyKanaKanjiConverter")
      ]
    ),
  ]
)
