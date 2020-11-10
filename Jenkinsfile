pipeline {
    agent none
    environment {
        CONAN_USE_CHANNEL = getConanChannel(env.BRANCH_NAME)
        CONAN_USE_USER = "josh"
        PKG_VER = getConanfileVersion()
        PKG_NAME = getRootJobName(env.JOB_NAME)
    }
    stages {
        stage('Clone respository') {
            agent any
            steps {
                checkout scm
            }
        }
        stage('Verify conan') {
            steps {
                conan_verify()
            }
        }
        stage('Export recipe') {
            steps {
                conan_export_recipe()
            }
        }
        stage('Install Dependencies') {
            steps {
                conan_install_()
            }
        }
        stage('Build') {
            steps {
                conan_build()
            }
        }
        stage('Package') {
            steps {
                conan_package()
            }
        }
        stage('Artifact') {
            steps {
                archiveArtifacts artifacts: "${env.PKG_NAME}/**", fingerprint: true
            }
        }
        stage("Export") {
            steps {
                conan_export_pkg()
            }
        }
        stage('Parallel post-build steps') {
            parallel {
                stage('Linux') {
                    agent {
                        docker {
                            image 'josh/docker-linux-agent:latest'
                            label 'linux'
                            args '-u 0 -v /var/run/docker.sock:/var/run/docker.sock'
                        }
                    }
                    steps {
                        sh 'Run unit tests here'
                    }
                }
                stage('Linux (Android)') {
                    agent {
                        docker {
                            image 'josh/docker-linux-agent:latest'
                            label 'linux'
                            args '-u 0 -v /var/run/docker.sock:/var/run/docker.sock'
                        }
                    }
                    environment {
                        PKG_VER = getConanfileVersion()
                    }
                    stages {
                        stage('Build x86') {
                            steps {
                                sh get_conan_android_install("-s arch=x86")
                            }
                        }
                        stage('Build x86_64') {
                            steps {
                                sh get_conan_android_install("-s arch=x86_64")
                            }
                        }
                        stage('Build armv7') {
                            steps {
                                sh get_conan_android_install("-s arch=armv7")
                            }
                        }
                        stage('Build armv8') {
                            steps {
                                sh get_conan_android_install("-s arch=armv8")
                            }
                        }
                    }
                }
                stage('Windows') {
                    agent {
                        label 'windows'
                    }
                    steps {
                        bat 'Run unit tests here'
                    }
                }
            }
        }
        stage('Upload') {
            steps {
                script {
                    conan_upload()
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

def getConanfileVersion() {
    def ver_header = "version: "
    def ret = "python conan inspect .".execute()
    ret.in.eachLine { line ->
        if (line.contains(ver_header)) {
            return line.minus(ver_header)
        }
    }
    return "0.0.0"
}

def runPythonCmd(cmd) {
    def py_cmd = "python " + cmd
    def ret = py_cmd.execute()
    ret.in.eachLine { line ->
        println(line)
    }
}

def runCmd(cmd) {
    if (System.properties['os.name'].toLowerCase().contains('windows')) {
        bat cmd
    } else {
        sh cmd
    }
}

def conan_verify() {
    runPythonCmd("conan")
}

def conan_export_recipe() {
    runPythonCmd("conan export . ${env.CONAN_USE_USER}/${env.CONAN_USE_CHANNEL}")
}

def conan_export_pkg() {
    runPythonCmd("conan export-pkg . ${env.CONAN_USE_USER}/${env.CONAN_USE_CHANNEL} -bf=build --force")
}

def conan_install_() {
    runPythonCmd('conan install . -if=build --build=outdated -s cppstd=17')
}

def get_conan_android_install(add_args) {
    return "conan install ${env.PKG_NAME}/${env.PKG_VER} --profile=profiles/android --build=outdated -o shared=True -s cppstd=17 -o lib_only=True " + add_args
}

def conan_build() {
    runPythonCmd('conan build . -bf=build')
}

def conan_package() {
    runPythonCmd("conan package . -bf=build -pf=${env.PKG_NAME}")
}

def conan_upload() {
    withCredentials([usernamePassword(credentialsId: 'jenkins_conan', usernameVariable: 'CONAN_LOGIN_USERNAME', passwordVariable: 'CONAN_PASSWORD')]) {
        runPythonCmd('conan user -p -r=omv')
        runPythonCmd('conan upload "*" -r omv --confirm --parallel --all --force --retry 6 --retry-wait 10')
    }
}