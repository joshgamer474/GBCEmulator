pipeline {
    agent none
    environment {
        CONAN_USE_CHANNEL = getConanChannel(env.BRANCH_NAME)
        USE_JOB_NAME = getRootJobName(env.JOB_NAME)
    }
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
                                sh "conan package . -bf=build -pf=${env.USE_JOB_NAME}"
                            }
                        }

                        stage('Artifact') {
                            steps {
                                archiveArtifacts artifacts: "${env.USE_JOB_NAME}/**", fingerprint: true
                            }
                        }

                        stage("Export") {
                            steps {
                                sh "conan export-pkg . josh/${env.CONAN_USE_CHANNEL} -bf=build --force"
                            }
                        }

                        stage('Upload') {
                            steps {
                                sh 'conan upload "*" -r omv --confirm --parallel'
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
                                bat "conan package . -bf=build -pf=${env.USE_JOB_NAME}"
                            }
                        }

                        stage('Artifact') {
                            steps {
                                archiveArtifacts artifacts: "${env.USE_JOB_NAME}/**", fingerprint: true
                            }
                        }

                        stage("Export") {
                            steps {
                                bat "conan export-pkg . josh/${env.CONAN_USE_CHANNEL} -bf=build --force"
                            }
                        }

                        stage('Upload') {
                            steps {
                                script {
                                    withCredentials([usernamePassword(credentialsId: 'jenkins_conan', usernameVariable: 'CONAN_LOGIN_USERNAME', passwordVariable: 'CONAN_PASSWORD')]) {
                                        bat 'conan user -p -r=omv'
                                        bat 'conan upload "*" -r omv --confirm --parallel'
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

def getConanChannel(branchName) {
    if ("master".equals(branchName)) {
        return "stable"
    } else {
        return branchName
    }
}

def getRootJobName(jobName) {
    String[] splt = jobName.split('/')
    return splt[0]
}