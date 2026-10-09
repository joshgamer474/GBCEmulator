#!/bin/sh
conan install . -pr:b=default -pr:h=profiles/ios --build=missing -of=build/ios
conan build . -pr:b=default -pr:h=profiles/ios -of=build/ios