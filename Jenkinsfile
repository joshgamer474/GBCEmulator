pipeline {
    agent none
    stages {
        stage('Clone respository') {
            agent any
            steps {
                checkout scm
            }
        }
        stage('Parallel build steps') {
            parallel {
                stage('Build on Linux') {
                    agent {
                        docker {
                            image 'josh/docker-linux-agent:latest'
                            label 'linux'
                            args '-u 0 -v /var/run/docker.sock:/var/run/docker.sock'
                        }
                    }
                    stages {
                        stage('Verify conan') {
                            steps {
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
                                sh 'conan package . -bf=build -pf=GBCEmulator'
                            }
                        }

                        stage('Artifact') {
                            steps {
                                archiveArtifacts artifacts: 'GBCEmulator/**', fingerprint: true
                            }
                        }
                    }
                }
                stage('Build on Windows') {
                    agent {
                        label 'windows'
                    }
                    stages {
                        stage('Verify conan') {
                            steps {
                                bat 'conan'
                            }
                        }

                        stage('Install Dependencies') {
                            steps {
                                bat 'conan install . -if=build --build=outdated -s cppstd=17'
                            }
                        }

                        stage('Build') {
                            steps {
                                bat 'conan build . -bf=build'
                            }
                        }

                        stage('Package') {
                            steps {
                                bat 'conan package . -bf=build -pf=GBCEmulator'
                            }
                        }

                        stage('Artifact') {
                            steps {
                                archiveArtifacts artifacts: 'GBCEmulator/**', fingerprint: true
                            }
                        }
                    }
                }
            }
        }
    }
}