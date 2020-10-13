pipeline {
    agent {
        docker {
            image 'josh/docker-linux-agent:latest'
            args '-u 0 -v /var/run/docker.sock:/var/run/docker.sock'
        }
    }
    stages {
        stage('Clone repository') {
            steps {
                checkout scm
            }
        }

        stage('Initialize conan') {
            steps {
                sh 'mkdir /.conan'
                sh 'conan'
            }
        }

        stage('Install Dependencies') {
            steps {
                sh 'conan install . -if=build --build=outdated -s cppstd=17'
            }
        }

        stage('Build') {
            steps {
                sh 'conan build . -bf=build'
            }
        }

        stage('Package') {
            steps {
                sh 'conan package -bf=build -pf=package'
            }
        }

        stage('Artifact') {
            steps {
                archiveArtifacts artifacts: 'package/**', fingerprint: true
            }
        }
    }
}