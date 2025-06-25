
#define _MSC_VER
#pragma comment(lib, "ssh-bcc.lib")
#pragma comment(lib, "libcurl-bcc-x64.lib")
#ifndef SSHPersistentSession_H
#define SSHPersistentSession_H
#include "libssh/libssh.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
/*
README: You must install libssh lib, follow guideline at T.B.D
*/
class SSHPersistentSession {
private:
    ssh_session mSession;
    bool mIsConnected = false;

public:
    SSHPersistentSession(const char* host, const char* user, const char* pass) : mSession(nullptr) {
        mSession = ssh_new();
        if (!mSession) throw std::runtime_error("Failed to create mSession");

        ssh_options_set(mSession, SSH_OPTIONS_HOST, host);
        ssh_options_set(mSession, SSH_OPTIONS_USER, user);
        int timeoutSecs = 1;
        ssh_options_set(mSession, SSH_OPTIONS_TIMEOUT, &timeoutSecs);

        if (ssh_connect(mSession) != SSH_OK)
            throw std::runtime_error(ssh_get_error(mSession));

        if (ssh_userauth_password(mSession, nullptr, pass) != SSH_AUTH_SUCCESS) {
            std::string err = ssh_get_error(mSession);
            ssh_disconnect(mSession);
            ssh_free(mSession);
            throw std::runtime_error("Auth failed: " + err);
        }

        // std::cout << "SSH mSession connected and authenticated.\n";
        mIsConnected = true;
    }

    ~SSHPersistentSession() {
        if (mSession) {
            ssh_disconnect(mSession);
            ssh_free(mSession);
        }
    }

    bool run_command(const char* command, std::string& output, int timeout_ms = 1000) {
        if (mIsConnected == false) {
            // std::cout << "SSH mSession is not connected.\n";
            return false; // it requires a reconnect explicitly
        }
        // if (ssh_is_connected(mSession) != 1) {
        //     std::cout << "SSH mSession is not connected.\n";
        //     return false;
        // }
        ssh_channel channel = ssh_channel_new(mSession);
        if (!channel) {
            std::cout << "Failed to allocate channel\n";
            return false;
        }
    
        bool success = false;
    
        do {
            if (ssh_channel_open_session(channel) != SSH_OK) {
                std::cout << "Failed to open channel: " << ssh_get_error(mSession) << "\n";
                mIsConnected = false; // it requires a reconnect explicitly
                break;
            }
    
            if (ssh_channel_request_exec(channel, command) != SSH_OK) {
                std::cout << "Failed to exec: " << ssh_get_error(mSession) << "\n";
                break;
            }
    
            output.clear();
            char buffer[256];
    
            // 1st read with timeout
            int nbytes = ssh_channel_read_timeout(channel, buffer, sizeof(buffer), 0, timeout_ms);
            if (nbytes == SSH_ERROR) {
                std::cout << "Read error: " << ssh_get_error(mSession) << "\n";
                break;
            } 
            // else if (nbytes == 0) { //FIXME: ChatGPT lied me.
            //     std::cout << "Timeout: no response within " << timeout_ms << " ms\n";
            //     break;
            // }
            else {
                output.append(buffer, nbytes);
                // Continue reading until EOF
                while ((nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0)) > 0)
                    output.append(buffer, nbytes);
            }
    
            success = true;
    
        } while (false);
    
        // Clean up in all cases
        if (channel) {
            ssh_channel_send_eof(channel);
            ssh_channel_close(channel);
            ssh_channel_free(channel);
        }
    
        return success;
    }
    
};

#endif