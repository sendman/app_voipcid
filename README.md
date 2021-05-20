# app_voipcid
Push client to VOIPCId

Client for VOIP CID push available at AppStore

    Installation on Asterisk server

    Install and configure for Asterisk:

    1.1. Download this repository and install required libs. Normally this should be already installed.

    1.2. Compile and install:

        $ cd app_voipcid
        $ make all install install_config

    1.3. Configure voipcid.conf with your FQDN in domain line

    2. Configure asterisk

    2.1. Create a macro in your extensions.conf, like this:

        [macro-push]
        exten => s,1,VOIPCId()

    2.2. Add a call to this macro in your respective dialplan

        exten => _X.,n,Macro(push)
