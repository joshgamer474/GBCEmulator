pipeline {
    agent none
    stages {
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
                        stage('Clone repository') {
                            steps {
                                checkout scm
                            }
                        }

                        stage('Test conan') {
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
                        docker {
                            image 'josh/docker-windows-agent:latest'
                            label 'windows'
                        }
                    }
                    stages {
                        stage('Clone repository') {
                            steps {
                                checkout scm
                            }
                        }

                        stage('Test conan') {
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
            }
        }
    }
}