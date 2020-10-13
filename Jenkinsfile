pipeline {
    agent {
        docker { image 'josh/docker-linux-agent:latest' }
    }
    stages {
        stage('Initialize') {
            steps {
                script {
                    def dockerHome = tool 'mydocker'
                    env.PATH = "${dockerHome}/bin:${env.PATH}"
                }
            }
        }

        stage('Clone repository') {
            steps {
                checkout scm
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