@Library('iac-lib@master')

package com.intel.epgsw

import groovy.util.XmlParser
import groovy.util.Node
import groovy.util.NodeList
import groovy.xml.XmlUtil

class CloneTester {
    def steps
    CloneTester( steps ) {
        this.steps = steps
     }
    void CheckoutMixedSubmodules() {
      def GITHUB_CRED_ID    = '8d646abf-dc6e-4149-9ad7-88b6348bfb40'
      def TEAMFORGE_CRED_ID = 'd0bf6cd6-8526-489d-aa03-bb23bfb04364'
      steps.git( branch: "mev_ts-9.3", 
                changelog: false, 
                credentialsId: GITHUB_CRED_ID, 
                poll: false, 
                url: "https://github.com/intel-innersource/networking.ethernet.acceleration.vswitch.p4-sde.p4-driver"
                )
      steps.checkOutMixedSubmodules steps: steps, 
                      gitHubCredId: GITHUB_CRED_ID,
                      teamForgeCredId: TEAMFORGE_CRED_ID
}

}

// def clone = new CloneTester()


def CheckoutMixed(reponame,branch,relativedir) {
  def GITHUB_CRED_ID    = '8d646abf-dc6e-4149-9ad7-88b6348bfb40'
  def TEAMFORGE_CRED_ID = 'd0bf6cd6-8526-489d-aa03-bb23bfb04364'
  // GitRepoClone(
  //     credentials: GITHUB_CRED_ID,
  //     path: "${relativedir}",
  //     fallbackBranch: "${branch}",
  //     url: "https://github.com/intel-innersource/networking.ethernet.acceleration.vswitch.p4-sde.${reponame}"
  // )
  checkOutMixedSubmodules(
    gitHubCredId: GITHUB_CRED_ID,
    teamForgeCredId: TEAMFORGE_CRED_ID
  )
}
    

def CheckOutRepo (reponame,branch,relativedir){
    print "branch is $branch ======="
    checkout([
        $class: 'GitSCM', branches: [[name: "$branch" ]],
          // doGenerateSubmoduleConfigurations: false,
          extensions: [
            [$class: 'RelativeTargetDirectory', relativeTargetDir: "${relativedir}"],
            [$class: 'CloneOption', depth: 0, noTags: true, shallow: true],
            // [$class: 'SubmoduleOption', recursiveSubmodules: false, parentCredentials: false],
            [$class: 'CleanBeforeCheckout'],
          ],
          gitTool: 'Default', submoduleCfg: [],
          userRemoteConfigs: [[
            credentialsId: '8d646abf-dc6e-4149-9ad7-88b6348bfb40',url: "https://github.com/intel-innersource/networking.ethernet.acceleration.vswitch.p4-sde.${reponame}"
          ]]
        ])
    
};

def CheckOutRepoWithSM (reponame,branch,relativedir){
    print "branch is $branch ======="
    checkout([
        $class: 'GitSCM', branches: [[name: "$branch" ]],
          doGenerateSubmoduleConfigurations: false,
          extensions: [
            [$class: 'RelativeTargetDirectory', relativeTargetDir: "${relativedir}"],
            [$class: 'CloneOption', depth: 0, noTags: true, shallow: true],
            [$class: 'SubmoduleOption', recursiveSubmodules: true, parentCredentials: true],
            [$class: 'CleanBeforeCheckout'],
          ],
          gitTool: 'Default', submoduleCfg: [],
          userRemoteConfigs: [[
            credentialsId: '8d646abf-dc6e-4149-9ad7-88b6348bfb40',
            // refspec: "+refs/pull/*/head:refs/remotes/origin/PR-*",
            url: "https://github.com/intel-innersource/networking.ethernet.acceleration.vswitch.p4-sde.${reponame}"
          ]]
        ])

};

def CheckOutTFRepo (String server,reponame,branch,relativedir){
    print "branch is $branch ======="
    checkout([
        $class: 'GitSCM', branches: [[name: "$branch" ]],
          doGenerateSubmoduleConfigurations: false,
          extensions: [
            [$class: 'RelativeTargetDirectory', relativeTargetDir: "${relativedir}"],
            [$class: 'CloneOption', depth: 0, noTags: true, shallow: true],
            [$class: 'SubmoduleOption', recursiveSubmodules: true, parentCredentials: true],
            [$class: 'CleanBeforeCheckout'],
          ],
          gitTool: 'Default', submoduleCfg: [],
          userRemoteConfigs: [[
            credentialsId: 'd0bf6cd6-8526-489d-aa03-bb23bfb04364',url: "ssh://$server:29418/$reponame"
          ]]
        ])
    
};

pipeline {
  options {
        buildDiscarder(logRotator(daysToKeepStr: '7', artifactDaysToKeepStr: '7'))
        skipDefaultCheckout() 
    }

  agent { 
    label 'BDCDELLNAT24' 
    // label "ND-CI-Fedora30-ia32e"
    }
  environment { 
        // package_name="p4sde_ts-9.3_PR${env.BUILD_NUMBER}"
        package_name="p4sde_ts-9.3_${env.BRANCH_NAME}_${env.BUILD_NUMBER}"
        HTTP_PROXY='http://proxy-dmz.intel.com:912'
        HTTPS_PROXY='http://proxy-dmz.intel.com:912'
    }

  stages {
          stage('Clean workspace') {
            steps {
                cleanWs()
              }
            }
            
          stage('Checkout driver from SCM') {
              steps {
                  dir('p4-sde/p4_sde-nat-p4-driver') {
                      checkout scm
                  }
                }
              }
    
          stage ("drivers submodule fetch") {
              steps {
                  sh '''
                  echo $WORKSPACE
                  cd $WORKSPACE/p4-sde/p4_sde-nat-p4-driver
                  git submodule update --init --recursive
                  cd ..
                  '''
              }
          }
      
    stage ("Checkout Repos") {
            steps {
                // CheckOutRepo("p4-driver", "mev_ts-9.3", "p4-sde/p4_sde-nat-p4-driver")
                CheckOutRepoWithSM("bf-syslibs", "mev_ts-9.3", "p4-sde/bf-syslibs")
                CheckOutRepoWithSM("bf-utils", "mev_ts-9.3", "p4-sde/bf-utils")
                CheckOutTFRepo("git-amr-2.devtools.intel.com", "nd_linux-common", "master", "p4-sde/nd_linux-common")
                sh '''
                echo $PWD
                ls -al
                '''
            } 
        }

    //  stage ("check-patch") {
    //         steps {
    //             sh '''
    //             cd $WORKSPACE/p4_sde-nat-p4-driver
    //             ./autogen.sh
    //             ./configure
    //             make checkpatch
    //             '''
    //         }
    //     }
      stage ("Build DPDK code") {
          steps {
              sh '''
              $WORKSPACE/p4-sde/p4_sde-nat-p4-driver/p4sde_build.sh
              '''
          }
      }
      stage ("Build MEV code") {
          steps {
              sh '''
              $WORKSPACE/p4-sde/p4_sde-nat-p4-driver/p4sde_build.sh --enable_mev
              '''
          }
      }
      stage ("Package Build") {
          steps {
              sh '''
              cp ${WORKSPACE}/p4-sde/p4_sde-nat-p4-driver/ci/createpkg.sh $WORKSPACE/p4-sde/createpkg.sh
              cd ${WORKSPACE}/p4-sde
              ${WORKSPACE}/p4-sde/createpkg.sh "${package_name}"
              '''
          }
      }
	stage ('upload') {
            steps {
    		rtUpload (
                        serverId: 'natp4_components-ir-local',
                        spec: """{
                            "files": [
                                    {
                                        "pattern": "p4-sde/${package_name}.tar.gz",
                                        "target": "natp4_components-ir-local/p4-sde/PR/${env.BRANCH_NAME}/${env.BUILD_NUMBER}/",
                                        "props": "retention.days=2555"
                                    }
                                ]
                        }""",
                    )
            }
        }
  }
      post {
        success { 
            echo "PASSED"
            archiveArtifacts "p4-sde/${package_name}.tar.gz"
        }
        failure { 
            cleanWs()
        }
      }
}
