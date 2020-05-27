node {
    stage('Clone repository') {
        checkout scm
    }

    stage('Install Dependencies') {
        sh 'conan install . -if=build --build=outdated -s cppstd=17'
    }

    stage('Build') {
        sh 'conan build . -bf=build'
    }

    stage('Package') {
        sh 'conan package -bf=build -pf=package'
    }

    stage('Artifact') {
        archiveArtifacts artifacts: 'package/**', fingerprint: true
    }
}