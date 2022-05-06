#!/bin/bash
conan install . -if=ax86build --profile:host=profiles/androidx86 --profile:build=profiles/default --build=outdated -o shared=True -o lib_only=True -s compiler.cppstd=17
conan install . -if=ax86_64build --profile:host=profiles/androidx86_64 --profile:build=profiles/default --build=outdated -o shared=True -o lib_only=True -s compiler.cppstd=17
conan install . -if=armv7build --profile:host=profiles/androidarmv7 --profile:build=profiles/default --build=outdated -o shared=True -o lib_only=True -s compiler.cppstd=17
conan install . -if=armv8build --profile:host=profiles/androidarmv8 --profile:build=profiles/default --build=outdated -o shared=True -o lib_only=True -s compiler.cppstd=17

conan build . -bf=ax86build
conan build . -bf=ax86_64build
conan build . -bf=armv7build
conan build . -bf=armv8build

conan package . -bf=ax86build -pf=apkg
conan package . -bf=ax86_64build -pf=apkg
conan package . -bf=armv7build -pf=apkg
conan package . -bf=armv8build -pf=apkg

cp -r apkg/lib ../gbcemulatorflutterapp/android/app/conan_deploy
