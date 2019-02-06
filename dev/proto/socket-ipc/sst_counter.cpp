/**
Simple 4-bit Up-Counter Model with one clock
*/

#include "sst_counter.h"

#include <sst/core/sst_config.h>
#include <arpa/inet.h> // inet_ntoa


// Component Constructor
sst_counter::sst_counter(SST::ComponentId_t id, SST::Params &params) : SST::Component(id) {

    // Initialize output
    m_output.init("module-" + getName() + "-> ", 1, 0, SST::Output::STDOUT);

    m_port = params.find<uint16_t>("port", 2000);
    m_sysc_counter = params.find<std::string>("sysc_counter", "");

    // Just register a plain clock for this simple example
    registerClock("500MHz", new SST::Clock::Handler<sst_counter>(this, &sst_counter::tick));

    // Tell SST to wait until we authorize it to exit
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

}

sst_counter::~sst_counter() {
//    close(m_newsockfd);
//    close(m_sockfd);
}

// setup is called by SST after a component has been constructed and should be used
// for initialization of variables
void sst_counter::setup() {

    m_output.verbose(CALL_INFO, 1, 0, "Component is being set up.\n");

    m_num_procs = 4;

//    add new socket to array of sockets
    for (int i = 0; i < m_num_procs; i++) {

        m_client_socks[i] = {
                {"fd",   0},
                {"port", 0},
                {"pid",  0}
        };

    }

    //a message
    std::string message("STOP");

    //create a master socket
    if (!(m_master_sock = socket(AF_INET, SOCK_STREAM, 0))) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //set master socket to allow multiple connections ,
    //this is just a good habit, it will work without this
    const int opt = 1;
    if (setsockopt(m_master_sock, SOL_SOCKET, SO_REUSEADDR, (char *) &opt,
                   sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // type of socket created
    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = INADDR_ANY;
    m_addr.sin_port = htons(m_port);

    // bind the socket to localhost port 8888
    if (bind(m_master_sock, (struct sockaddr *) &m_addr, sizeof(m_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d\n", m_port);

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(m_master_sock, m_num_procs * 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // accept the incoming connection
    int addrlen = sizeof(m_addr);
    puts("Waiting for connections ...");

    ///////////////////////////////////////////////


    for (int i = 0; i < m_num_procs; i++) {

        m_pids.push_back(fork());

        if (!m_pids.back()) {

            char *args[] = {&m_sysc_counter[0u], &(to_string(m_port))[0u], nullptr};
            m_output.verbose(CALL_INFO, 1, 0,
                             "Forking process %s (pid: %d)\n", args[0], getpid());
            execvp(args[0], args);

        }
    }


    //////////////////////////////////////////////

    int sd, max_sd;
    int valread;
    int connected_procs = 0;

    std::cout << m_client_socks << std::endl;

    while (connected_procs < m_num_procs) {

        //clear the socket set
        FD_ZERO(&m_read_fds);

        //add master socket to set
        FD_SET(m_master_sock, &m_read_fds);
        max_sd = m_master_sock;

        //add child sockets to set
        for (int i = 0; i < m_num_procs; i++) {
            //socket descriptor
            sd = m_client_socks[i]["fd"];

            //if valid socket descriptor then add to read list
            if (sd > 0) {
                FD_SET(sd, &m_read_fds);
            }

            //highest file descriptor number, need it for the select function
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // wait for an activity on one of the sockets , timeout is nullptr ,
        // so wait indefinitely
        if ((select(max_sd + 1, &m_read_fds, nullptr, nullptr, nullptr) < 0)
            && (errno != EINTR)) {
            printf("select error");
        }

        // If something happened on the master socket, then its an incoming connection
        if (FD_ISSET(m_master_sock, &m_read_fds)) {

            if ((m_new_sock = accept(m_master_sock, (struct sockaddr *) &m_addr,
                                     (socklen_t * ) & addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d\n",
                   m_new_sock, inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));

            if (!(valread = read(m_new_sock, m_buffer, BUFSIZE))) {

                //Somebody disconnected, get his details and print
                getpeername(m_new_sock, (struct sockaddr *) &m_addr, (socklen_t * ) & addrlen);
                printf("Client disconnected, ip %s , port %d \n",
                       inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));

                //Close the socket and mark as 0 in list for reuse
                close(m_new_sock);
                m_client_socks[connected_procs]["fd"] = 0;


            } else {

                getpeername(m_new_sock, (struct sockaddr *) &m_addr, (socklen_t * ) & addrlen);
                m_buffer[valread] = '\0';

                m_client_socks[connected_procs]["fd"] = m_new_sock;
                m_client_socks[connected_procs]["port"] = ntohs(m_addr.sin_port);
                m_client_socks[connected_procs]["pid"] = json::parse(m_buffer)["pid"];
                connected_procs++;

            }


        }


    }

    std::cout << m_client_socks << std::endl;
//    //else its some IO operation on some other socket
//    int i = 0;
//
//    while (i < m_num_procs) {
//
//        sd = m_client_socks[i];
//
////        // If something happened on the master socket, then its an incoming connection
////        if (FD_ISSET(m_master_sock, &m_read_fds)) {
////
////            //send new connection greeting message
////            if (write(sd, message.c_str(), message.length()) != message.length()) {
////                perror("send");
////            }
////
////            printf("Welcome message sent successfully\n");
////
////        }
//
//        if (FD_ISSET(sd, &m_read_fds)) {
//            printf("HERE NOW [%d]: %d\n", i, sd);
//            //Check if it was for closing , and also read the
//            //incoming message
//            if (!(valread = read(sd, m_buffer, 1024))) {
//
//                //Somebody disconnected, get his details and print
//                getpeername(sd, (struct sockaddr *) &m_addr, (socklen_t * ) & addrlen);
//                printf("Client disconnected, ip %s , port %d \n",
//                       inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
//
//                //Close the socket and mark as 0 in list for reuse
//                close(sd);
//                m_client_socks[i] = 0;
//
//            } else {  // Echo back the message that came in
//
//                getpeername(sd, (struct sockaddr *) &m_addr, (socklen_t * ) & addrlen);
//                printf("YO ip %s , port %d \n",
//                       inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port));
//                printf("BUF: %s", m_buffer);
//
////                m_buffer[valread] = '\0';
//                write(sd, message.c_str(), message.length());
//                i++;
//
//            }
//
//        }
//    }

    //////////////////////////////////////////////

}

// finish is called by SST before the simulation is ended and should be used
// to clean up variables and memory
void sst_counter::finish() {
    m_output.verbose(CALL_INFO, 1, 0, "Component is being finished.\n");
}

// clockTick is called by SST from the registerClock function
// this function runs once every clock cycle
bool sst_counter::tick(SST::Cycle_t current_cycle) {


    ////////////////////////////////////////////


    /////////////////////////////////////////////////

//
//    if (done) {
//
//        // ---------------- SYSTEMC MODULE TESTBENCH ----------------
//
//        // assign SST clock to SystemC clock
//        m_data_out["clock"] = current_cycle;
//
//        // set reset to 1 at 4 ns
//        if (current_cycle == 4) {
//            std::cout << "RESET ON" << std::endl;
//            m_data_out["reset"] = 1;
//        }
//
//        // set reset to 0 at 8 ns
//        if (current_cycle == 8) {
//            std::cout << "RESET OFF" << std::endl;
//            m_data_out["reset"] = 0;
//        }
//
//        // set enable to 1 at 12 ns
//        if (current_cycle == 12) {
//            std::cout << "ENABLE ON" << std::endl;
//            m_data_out["enable"] = 1;
//        }
//
//        // set enable to 0 at 50 ns
//        if (current_cycle == 50) {
//            std::cout << "ENABLE OFF" << std::endl;
//            m_data_out["enable"] = 0;
//        }
//
//        // turn module off at 52 ns
//        if (current_cycle == 52) {
//            std::cout << "MODULE OFF" << std::endl;
//            m_data_out["on"] = false;
//        }
//
//        // ---------------- SOCKET COMMUNICATION ----------------
//
//        // ---------------- WRITE DATA ----------------
//        send_signal(m_data_out, m_newsockfd);
//
//        // if module is on, dump the JSON object buffer
//        if (m_data_out["on"]) {
//
//            // ---------------- READ DATA ----------------
//            m_data_in = recv_signal(m_buffer, m_newsockfd);
//
//            m_output.verbose(CALL_INFO, 1, 0, "Counter: %s \n",
//                             std::string(m_data_in["counter_out"]).c_str());
//
//        }
//    }

    return false;

}
