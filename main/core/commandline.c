// command.c

#include "core/commandline.h"
#include "core/callbacks.h"
#include "esp_sntp.h"
#include "managers/ap_manager.h"
#include "managers/ble_manager.h"
#include "managers/dial_manager.h"
#include "managers/rgb_manager.h"
#include "managers/settings_manager.h"
#include "managers/wifi_manager.h"
#include "vendor/pcap.h"
#include "vendor/printer.h"
#include <esp_timer.h>
#include <managers/gps_manager.h>
#include <managers/views/terminal_screen.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <vendor/dial_client.h>
#include "esp_wifi.h"
#include "esp_console.h"

static CommandNode *command_list_head = NULL;
TaskHandle_t VisualizerHandle = NULL;

void command_init() { command_list_head = NULL; }

CommandFunction find_command(const char *name)
{
    CommandNode *current = command_list_head;
    while (current != NULL)
    {
        if (strcmp(current->name, name) == 0)
        {
            return current->function;
        }
        current = current->next;
    }
    return NULL;
}

int cmd_wifi_scan_start(int argc, char **argv)
{
    wifi_manager_start_scan();
    wifi_manager_print_scan_results_with_oui();
    return 0;
}
esp_console_cmd_t start_wifi_scan_cmd ={
    .command = "scanap",
    .help = "scan for access points",
    .hint= NULL,
    .func= &cmd_wifi_scan_start,
};

int cmd_wifi_scan_stop(int argc, char **argv)
{
    wifi_manager_stop_monitor_mode();
    pcap_file_close();
    printf("WiFi scan stopped.\n");
    TERMINAL_VIEW_ADD_TEXT("WiFi scan stopped.\n");
    return 0;
}
esp_console_cmd_t stop_wifi_scan_cmd = {
    .command = "stopscan",
    .help = "stop scanning wifi",
    .hint=NULL,
    .func=&cmd_wifi_scan_stop
};

void cmd_wifi_scan_results(int argc, char **argv)
{
    printf("WiFi scan results displaying with OUI matching.\n");
    TERMINAL_VIEW_ADD_TEXT("WiFi scan results displaying with OUI matching.\n");
    wifi_manager_print_scan_results_with_oui();
}

int handle_list(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "-a") == 0)
    {
        cmd_wifi_scan_results(argc, argv);
        return 0;
    }
    else if (argc > 1 && strcmp(argv[1], "-s") == 0)
    {
        wifi_manager_list_stations();
        printf("Listed Stations...\n");
        TERMINAL_VIEW_ADD_TEXT("Listed Stations...\n");
        return 0;
    }
    else
    {
        printf("Usage: list -a (for Wi-Fi scan results)\n");
        TERMINAL_VIEW_ADD_TEXT("Usage: list -a (for Wi-Fi scan results)\n");
    }
    return 0;
}
esp_console_cmd_t handle_list_cmd={
    .command = "list",
    .help = "    Description: List Wi-Fi scan results or connected stations.\n"
    "    Usage: list -a | list -s\n"
    "    Arguments:\n"
    "        -a  : Show access points from Wi-Fi scan\n"
    "        -s  : List connected stations\n\n",
    .hint = NULL,
    .func = &handle_list,
};

int handle_beaconspam(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "-r") == 0)
    {
        printf("Starting Random beacon spam...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting Random beacon spam...\n");
        wifi_manager_start_beacon(NULL);
        return 0 ;
    }

    if (argc > 1 && strcmp(argv[1], "-rr") == 0)
    {
        printf("Starting Rickroll beacon spam...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting Rickroll beacon spam...\n");
        wifi_manager_start_beacon("RICKROLL");
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "-l") == 0)
    {
        printf("Starting AP List beacon spam...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting AP List beacon spam...\n");
        wifi_manager_start_beacon("APLISTMODE");
        return 0;
    }

    if (argc > 1)
    {
        wifi_manager_start_beacon(argv[1]);
        return 0;
    }
    else
    {
        printf("Usage: beaconspam -r (for Beacon Spam Random)\n");
        TERMINAL_VIEW_ADD_TEXT("Usage: beaconspam -r (for Beacon Spam Random)\n");
    }
    return 0;
}
esp_console_cmd_t beaconspam_cmd={
    .command="beaconspam",
    .help = "    Description: Start beacon spam with different modes.\n"
    "    Usage: beaconspam [OPTION]\n"
    "    Arguments:\n"
    "        -r   : Start random beacon spam\n"
    "        -rr  : Start Rickroll beacon spam\n"
    "        -l   : Start AP List beacon spam\n"
    "        [SSID]: Use specified SSID for beacon spam\n\n",
    .hint = NULL,
    .func = &handle_beaconspam,
};

int handle_stop_spam(int argc, char **argv)
{
    wifi_manager_stop_beacon();
    printf("Beacon Spam Stopped...\n");
    TERMINAL_VIEW_ADD_TEXT("Beacon Spam Stopped...\n");
    return 0;
}
esp_console_cmd_t stopspam_cmd = {
    .command="stopspam",
    .help = "    Description: Stop ongoing beacon spam.\n"
    "    Usage: stopspam\n\n",
    .hint=NULL,
    .func=&handle_stop_spam,
};

int handle_sta_scan(int argc, char **argv)
{
    wifi_manager_start_monitor_mode(wifi_stations_sniffer_callback);
    printf("Started Station Scan...\n");
    TERMINAL_VIEW_ADD_TEXT("Started Station Scan...\n");
    return 0;
}
esp_console_cmd_t wifi_sta_scan_cmd={
    .command="scansta",
    .help="scan for wifi stations",
    .hint = NULL,
    .func =&handle_sta_scan,
};

int handle_attack_cmd(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "-d") == 0)
    {
        printf("Deauth Attack Starting...\n");
        TERMINAL_VIEW_ADD_TEXT("Deauth Attack Starting...\n");
        wifi_manager_start_deauth();
        return 0;
    }
    else
    {
        printf("Usage: attack -d (for deauthing access points)\n");
        TERMINAL_VIEW_ADD_TEXT("Usage: attack -d (for deauthing access points)\n");
        return 0;
    }
}
esp_console_cmd_t attack_cmd = {
    .command = "attack",
    .help =
        "Launch an attack (e.g., deauthentication attack).\n"
        "Usage:\n"
        "  attack -d <SSID>  Start deauth attack on specified SSID\n"
        "Options:\n"
        "  -d          Start deauthentication attack\n",
    .hint = NULL,
    .func = &handle_attack_cmd,

};

int handle_stop_deauth(int argc, char **argv)
{
    wifi_manager_stop_deauth();
    printf("Deauthing Stopped....\n");
    TERMINAL_VIEW_ADD_TEXT("Deauthing Stopped....\n");
    return 0;
}
esp_console_cmd_t stopdeauth_cmd={
    .command = "stopdeauth",
    .help = 
    "    Description: Stop ongoing deauthentication attack.\n"
    "    Usage: stopdeauth\n\n",
    .hint = NULL,
    .func = & handle_stop_deauth
};

int handle_select_cmd(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: select -a <number>\n");
        TERMINAL_VIEW_ADD_TEXT("Usage: select -a <number>\n");
        return 0;
    }

    if (strcmp(argv[1], "-a") == 0)
    {
        char *endptr;

        int num = (int)strtol(argv[2], &endptr, 10);

        if (*endptr == '\0')
        {
            wifi_manager_select_ap(num);
        }
        else
        {
            printf("Error: is not a valid number.\n");
            TERMINAL_VIEW_ADD_TEXT("Error: is not a valid number.\n");
        }
    }
    else
    {
        printf("Invalid option. Usage: select -a <number>\n");
        TERMINAL_VIEW_ADD_TEXT("Invalid option. Usage: select -a <number>\n");
    }
    return 0;
}
esp_console_cmd_t select_cmd={
.command="select",
.help = "    Description: Select an access point by index from the scan results.\n"
    "    Usage: select -a <number>\n"
    "    Arguments:\n"
    "        -a  : AP selection index (must be a valid number)\n\n",
    .hint = NULL,
    .func= &handle_select_cmd,
};

int discover_task(void *pvParameter)
{
    DIALClient client;
    DIALManager manager;

    if (dial_client_init(&client) == ESP_OK)
    {

        dial_manager_init(&manager, &client);

        explore_network(&manager);

        dial_client_deinit(&client);
    }
    else
    {
        printf("Failed to init DIAL client.\n");
        TERMINAL_VIEW_ADD_TEXT("Failed to init DIAL client.\n");
    }

    vTaskDelete(NULL);
    return 0;
}
esp_console_cmd_t discover_cmd={
    .command="discover",
    .help="Description: Discover DIAL devices on the network.\n",
    .hint=NULL,
    .func=&discover_task,
};

void handle_stop_flipper(int argc, char **argv)
{
    wifi_manager_stop_deauth();
#ifndef CONFIG_IDF_TARGET_ESP32S2
    ble_stop();
#endif
    if (buffer_offset > 0)
    { // Only flush if there's data in buffer
        csv_flush_buffer_to_file();
    }
    csv_file_close();                  // Close any open CSV files
    gps_manager_deinit(&g_gpsManager); // Clean up GPS if active
    wifi_manager_stop_monitor_mode();  // Stop any active monitoring
    printf("Stopped activities.\nClosed files.\n");
    TERMINAL_VIEW_ADD_TEXT("Stopped activities.\nClosed files.\n");
}
esp_console_cmd_t stop_cmd={
    .command = "stop",
    .help = "",
    .func  = &handle_stop_flipper,
};

void handle_dial_command(int argc, char **argv)
{
    xTaskCreate(&discover_task, "discover_task", 10240, NULL, 5, NULL);

}
esp_console_cmd_t dialconnect_cmd={
    .command = "dialconnect",
    .help ="    Description: Cast a Random Youtube Video on all Smart TV's on "
           "your LAN (Requires You to Run Connect First)\n"
            "    Usage: dialconnect\n",
    .hint = NULL,
    .func = &handle_dial_command,
};

int handle_wifi_connection(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s \"<SSID>\" \"<PASSWORD>\"\n", argv[0]);
        TERMINAL_VIEW_ADD_TEXT("Usage: %s \"<SSID>\" \"<PASSWORD>\"\n", argv[0]);
        return 0;
    }

    char ssid_buffer[128] = {0};
    char password_buffer[128] = {0};
    const char *ssid = NULL;
    const char *password = "";

    // Handle SSID - could be spread across multiple arguments if it contains spaces
    int i = 1;
    if (argv[1][0] == '"')
    {
        // SSID is in quotes, need to concatenate until closing quote
        char *dest = ssid_buffer;
        bool found_end_quote = false;

        // Skip the opening quote
        strncpy(dest, &argv[1][1], sizeof(ssid_buffer) - 1);
        dest += strlen(&argv[1][1]);

        // Check if the closing quote is in the same argument
        if (argv[1][strlen(argv[1]) - 1] == '"')
        {
            ssid_buffer[strlen(ssid_buffer) - 1] = '\0'; // Remove closing quote
            found_end_quote = true;
        }

        // If not found in first arg, look in subsequent args
        i = 2;
        while (!found_end_quote && i < argc)
        {
            *dest++ = ' '; // Add space between arguments

            if (strchr(argv[i], '"'))
            {
                // This argument contains the closing quote
                size_t len = strchr(argv[i], '"') - argv[i];
                strncpy(dest, argv[i], len);
                dest[len] = '\0';
                found_end_quote = true;
            }
            else
            {
                // This argument is part of the SSID
                strncpy(dest, argv[i], sizeof(ssid_buffer) - (dest - ssid_buffer) - 1);
                dest += strlen(argv[i]);
            }
            i++;
        }

        if (!found_end_quote)
        {
            printf("Error: Missing closing quote for SSID\n");
            TERMINAL_VIEW_ADD_TEXT("Error: Missing closing quote for SSID\n");
            return 0;
        }

        ssid = ssid_buffer;
    }
    else
    {
        // SSID is a single argument without quotes
        ssid = argv[1];
        i = 2;
    }

    // Handle password if provided
    if (i < argc)
    {
        if (argv[i][0] == '"')
        {
            // Password is in quotes
            char *dest = password_buffer;
            bool found_end_quote = false;

            // Skip the opening quote
            strncpy(dest, &argv[i][1], sizeof(password_buffer) - 1);
            dest += strlen(&argv[i][1]);

            // Check if the closing quote is in the same argument
            if (argv[i][strlen(argv[i]) - 1] == '"')
            {
                password_buffer[strlen(password_buffer) - 1] = '\0'; // Remove closing quote
                found_end_quote = true;
            }

            // If not found in first arg, look in subsequent args
            i++;
            while (!found_end_quote && i < argc)
            {
                *dest++ = ' '; // Add space between arguments

                if (strchr(argv[i], '"'))
                {
                    // This argument contains the closing quote
                    size_t len = strchr(argv[i], '"') - argv[i];
                    strncpy(dest, argv[i], len);
                    dest[len] = '\0';
                    found_end_quote = true;
                }
                else
                {
                    // This argument is part of the password
                    strncpy(dest, argv[i], sizeof(password_buffer) - (dest - password_buffer) - 1);
                    dest += strlen(argv[i]);
                }
                i++;
            }

            if (!found_end_quote)
            {
                printf("Error: Missing closing quote for password\n");
                TERMINAL_VIEW_ADD_TEXT("Error: Missing closing quote for password\n");
                return 0;
            }

            password = password_buffer;
        }
        else
        {
            // Password is a single argument without quotes
            password = argv[i];
        }
    }

    if (strlen(ssid) == 0)
    {
        printf("SSID cannot be empty\n");
        TERMINAL_VIEW_ADD_TEXT("SSID cannot be empty\n");
        return 0;
    }

    printf("Connecting to SSID: %s\n", ssid);
    TERMINAL_VIEW_ADD_TEXT("Connecting to SSID: %s\n", ssid);

    wifi_manager_connect_wifi(ssid, password);

    if (VisualizerHandle == NULL)
    {
#ifdef WITH_SCREEN
        xTaskCreate(screen_music_visualizer_task, "udp_server", 4096, NULL, 5, &VisualizerHandle);
#else
        xTaskCreate(animate_led_based_on_amplitude, "udp_server", 4096, NULL, 5, &VisualizerHandle);
#endif
    }

#ifdef CONFIG_HAS_RTC_CLOCK
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
#endif
return 0;
}
esp_console_cmd_t connect_cmd={
    .command = "connect",
    .help = "    Description: Connects to Specific WiFi Network\n"
    "    Usage: connect <SSID> <Password>\n",
    .hint = NULL,
    .func = &handle_wifi_connection,
}; 

#ifndef CONFIG_IDF_TARGET_ESP32S2

int handle_ble_scan_cmd(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "-f") == 0)
    {
        printf("Starting Find the Flippers.\n");
        TERMINAL_VIEW_ADD_TEXT("Starting Find the Flippers.\n");
        ble_start_find_flippers();
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "-ds") == 0)
    {
        printf("Starting BLE Spam Detector.\n");
        TERMINAL_VIEW_ADD_TEXT("Starting BLE Spam Detector.\n");
        ble_start_blespam_detector();
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "-a") == 0)
    {
        printf("Starting AirTag Scanner.\n");
        TERMINAL_VIEW_ADD_TEXT("Starting AirTag Scanner.\n");
        ble_start_airtag_scanner();
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "-r") == 0)
    {
        printf("Scanning for Raw Packets\n");
        TERMINAL_VIEW_ADD_TEXT("Scanning for Raw Packets\n");
        ble_start_raw_ble_packetscan();
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "-s") == 0)
    {
        printf("Stopping BLE Scan.\n");
        TERMINAL_VIEW_ADD_TEXT("Stopping BLE Scan.\n");
        ble_stop();
        return 0;
    }

    printf("Invalid Command Syntax.\n");
    TERMINAL_VIEW_ADD_TEXT("Invalid Command Syntax.\n");
    return 0;
}

#endif

int handle_start_portal(int argc, char **argv)
{

    const char *URLorFilePath = settings_get_portal_url(&G_Settings);
    const char *SSID = settings_get_portal_ssid(&G_Settings);
    const char *Password = settings_get_portal_password(&G_Settings);
    const char *AP_SSID = settings_get_portal_ap_ssid(&G_Settings);
    const char *Domain = settings_get_portal_domain(&G_Settings);
    bool offlinemode = settings_get_portal_offline_mode(&G_Settings);

    const char *url = URLorFilePath;
    const char *ssid = SSID;
    const char *password = Password;
    const char *ap_ssid = AP_SSID;
    const char *domain = Domain;

    if (argc == 6)
    {
        url = (argv[1] && argv[1][0] != '\0') ? argv[1] : url;
        ssid = (argv[2] && argv[2][0] != '\0') ? argv[2] : ssid;
        password = (argv[3] && argv[3][0] != '\0') ? argv[3] : password;
        ap_ssid = (argv[4] && argv[4][0] != '\0') ? argv[4] : ap_ssid;
        domain = (argv[5] && argv[5][0] != '\0') ? argv[5] : domain;
    }
    else if (argc == 4)
    {
        url = (argv[1] && argv[1][0] != '\0') ? argv[1] : url;
        ap_ssid = (argv[2] && argv[2][0] != '\0') ? argv[2] : ap_ssid;
        domain = (argv[3] && argv[3][0] != '\0') ? argv[3] : domain;
    }
    else if (argc != 1)
    {
        printf("Error: Incorrect number of arguments.\n");
        TERMINAL_VIEW_ADD_TEXT("Error: Incorrect number of arguments.\n");
        printf("Usage: %s <URL> <SSID> <Password> <AP_ssid> <DOMAIN>\n", argv[0]);
        TERMINAL_VIEW_ADD_TEXT("Usage: %s <URL> <SSID> <Password> <AP_ssid> <DOMAIN>\n", argv[0]);
        printf("or\n");
        TERMINAL_VIEW_ADD_TEXT("or\n");
        printf("Usage: %s <filepath> <APSSID> <Domain>\n", argv[0]);
        TERMINAL_VIEW_ADD_TEXT("Usage: %s <filepath> <APSSID> <Domain>\n", argv[0]);
        return 0;
    }

    if (url == NULL || url[0] == '\0')
    {
        printf("Error: URL or File Path cannot be empty.\n");
        TERMINAL_VIEW_ADD_TEXT("Error: URL or File Path cannot be empty.\n");
        return 0;
    }

    if (ap_ssid == NULL || ap_ssid[0] == '\0')
    {
        printf("Error: AP SSID cannot be empty.\n");
        TERMINAL_VIEW_ADD_TEXT("Error: AP SSID cannot be empty.\n");
        return 0;
    }

    if (domain == NULL || domain[0] == '\0')
    {
        printf("Error: Domain cannot be empty.\n");
        TERMINAL_VIEW_ADD_TEXT("Error: Domain cannot be empty.\n");
        return 0;
    }

    if (ssid && ssid[0] != '\0' && password && password[0] != '\0' && !offlinemode)
    {
        printf("Starting portal with SSID: %s, Password: %s, AP_SSID: %s, Domain: "
               "%s\n",
               ssid, password, ap_ssid, domain);
        TERMINAL_VIEW_ADD_TEXT("Starting portal with SSID: %s, Password: %s, "
                               "AP_SSID: %s, Domain: %s\n",
                               ssid, password, ap_ssid, domain);
        wifi_manager_start_evil_portal(url, ssid, password, ap_ssid, domain);
    }
    else if (offlinemode)
    {
        printf("Starting portal in offline mode with AP_SSID: %s, Domain: %s\n", ap_ssid, domain);
        TERMINAL_VIEW_ADD_TEXT("Starting portal in offline mode with AP_SSID: %s, Domain: %s\n",
                               ap_ssid, domain);
        wifi_manager_start_evil_portal(url, NULL, NULL, ap_ssid, domain);
    }
    return 0;
}
esp_console_cmd_t  startportal_cmd={
    .command = "startportal",
    .help = "    Description: Start a portal with specified SSID and password.\n"
    "    Usage: startportal <URL> <SSID> <Password> <AP_ssid> <Domain>\n"
    "    Arguments:\n"
    "        <URL>       : URL for the portal\n"
    "        <SSID>      : Wi-Fi SSID for the portal\n"
    "        <Password>  : Wi-Fi password for the portal\n"
    "        <AP_ssid>   : SSID for the access point\n\n"
    "        <Domain>    : Custom Domain to Spoof In Address Bar\n\n"
    "  OR \n\n"
    "Offline Usage: startportal <FilePath> <AP_ssid> <Domain>\n",
    .hint = NULL,
    .func = &handle_start_portal,


};

bool ip_str_to_bytes(const char *ip_str, uint8_t *ip_bytes)
{
    int ip[4];
    if (sscanf(ip_str, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4)
    {
        for (int i = 0; i < 4; i++)
        {
            if (ip[i] < 0 || ip[i] > 255)
                return false;
            ip_bytes[i] = (uint8_t)ip[i];
        }
        return true;
    }
    return false;
}

bool mac_str_to_bytes(const char *mac_str, uint8_t *mac_bytes)
{
    int mac[6];
    if (sscanf(mac_str, "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4],
               &mac[5]) == 6)
    {
        for (int i = 0; i < 6; i++)
        {
            if (mac[i] < 0 || mac[i] > 255)
                return false;
            mac_bytes[i] = (uint8_t)mac[i];
        }
        return true;
    }
    return false;
}

void encrypt_tp_link_command(const char *input, uint8_t *output, size_t len)
{
    uint8_t key = 171;
    for (size_t i = 0; i < len; i++)
    {
        output[i] = input[i] ^ key;
        key = output[i];
    }
}

void decrypt_tp_link_response(const uint8_t *input, char *output, size_t len)
{
    uint8_t key = 171;
    for (size_t i = 0; i < len; i++)
    {
        output[i] = input[i] ^ key;
        key = input[i];
    }
}

int handle_tp_link_test(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: tp_link_test <on|off|loop>\n");
        TERMINAL_VIEW_ADD_TEXT("Usage: tp_link_test <on|off|loop>\n");
        return 0;
    }

    bool isloop = false;

    if (strcmp(argv[1], "loop") == 0)
    {
        isloop = true;
    }
    else if (strcmp(argv[1], "on") != 0 && strcmp(argv[1], "off") != 0)
    {
        printf("Invalid argument. Use 'on', 'off', or 'loop'.\n");
        return 0;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(9999);

    int iterations = isloop ? 10 : 1;

    for (int i = 0; i < iterations; i++)
    {
        const char *command;
        if (isloop)
        {
            command = (i % 2 == 0) ? "{\"system\":{\"set_relay_state\":{\"state\":1}}}" : // "on"
                          "{\"system\":{\"set_relay_state\":{\"state\":0}}}";             // "off"
        }
        else
        {

            command = (strcmp(argv[1], "on") == 0)
                          ? "{\"system\":{\"set_relay_state\":{\"state\":1}}}"
                          : "{\"system\":{\"set_relay_state\":{\"state\":0}}}";
        }

        uint8_t encrypted_command[128];
        memset(encrypted_command, 0, sizeof(encrypted_command));

        size_t command_len = strlen(command);
        if (command_len >= sizeof(encrypted_command))
        {
            printf("Command too large to encrypt\n");
            return 0;
        }

        encrypt_tp_link_command(command, encrypted_command, command_len);

        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock < 0)
        {
            printf("Failed to create socket: errno %d\n", errno);
            return 0;
        }

        int broadcast = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

        int err = sendto(sock, encrypted_command, command_len, 0, (struct sockaddr *)&dest_addr,
                         sizeof(dest_addr));
        if (err < 0)
        {
            printf("Error occurred during sending: errno %d\n", errno);
            TERMINAL_VIEW_ADD_TEXT("Error occurred during sending: errno %d\n", errno);
            close(sock);
            return 0;
        }

        printf("Broadcast message sent: %s\n", command);
        TERMINAL_VIEW_ADD_TEXT("Broadcast message sent: %s\n", command);

        struct timeval timeout = {2, 0};
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        uint8_t recv_buf[128];
        socklen_t addr_len = sizeof(dest_addr);
        int len = recvfrom(sock, recv_buf, sizeof(recv_buf) - 1, 0, (struct sockaddr *)&dest_addr,
                           &addr_len);
        if (len < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                printf("No response from any device\n");
                TERMINAL_VIEW_ADD_TEXT("No response from any device\n");
            }
            else
            {
                printf("Error receiving response: errno %d\n", errno);
                TERMINAL_VIEW_ADD_TEXT("Error receiving response: errno %d\n", errno);
            }
        }
        else
        {
            recv_buf[len] = 0;
            char decrypted_response[128];
            decrypt_tp_link_response(recv_buf, decrypted_response, len);
            decrypted_response[len] = 0;
            printf("Response: %s\n", decrypted_response);
        }

        close(sock);

        if (isloop && i < 9)
        {
            vTaskDelay(pdMS_TO_TICKS(700));
        }
    }
    return 0;
}
esp_console_cmd_t tplinktest_cmd={
    .command = "tplinktest",
    .help = "",
    .hint = NULL,
    .func =&handle_tp_link_test,

};


int handle_ip_lookup(int argc, char **argv)
{
    printf("Starting IP lookup...\n");
    TERMINAL_VIEW_ADD_TEXT("Starting IP lookup...\n");
    wifi_manager_start_ip_lookup();
    return 0;
}
esp_console_cmd_t scanlocal_cmd={
    .command="scanlocal",
    .help="Description: Start a Wi-Fi access point (AP) scan.\n",
    .hint=NULL,
    .func= &handle_ip_lookup

};

int handle_capture_scan(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Error: Incorrect number of arguments.\n");
        TERMINAL_VIEW_ADD_TEXT("Error: Incorrect number of arguments.\n");
        return 0;
    }

    char *capturetype = argv[1];

    if (capturetype == NULL || capturetype[0] == '\0')
    {
        printf("Error: Capture Type cannot be empty.\n");
        TERMINAL_VIEW_ADD_TEXT("Error: Capture Type cannot be empty.\n");
        return 0;
    }

    if (strcmp(capturetype, "-probe") == 0)
    {
        printf("Starting probe request\npacket capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting probe request\npacket capture...\n");
        int err = pcap_file_open("probescan", PCAP_CAPTURE_WIFI);

        if (err != ESP_OK)
        {
            printf("Error: pcap failed to open\n");
            TERMINAL_VIEW_ADD_TEXT("Error: pcap failed to open\n");
            return 0;
        }
        wifi_manager_start_monitor_mode(wifi_probe_scan_callback);
    }

    if (strcmp(capturetype, "-deauth") == 0)
    {
        int err = pcap_file_open("deauthscan", PCAP_CAPTURE_WIFI);

        if (err != ESP_OK)
        {
            printf("Error: pcap failed to open\n");
            TERMINAL_VIEW_ADD_TEXT("Error: pcap failed to open\n");
            return 0;
        }
        wifi_manager_start_monitor_mode(wifi_deauth_scan_callback);
    }

    if (strcmp(capturetype, "-beacon") == 0)
    {
        printf("Starting beacon\npacket capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting beacon\npacket capture...\n");
        int err = pcap_file_open("beaconscan", PCAP_CAPTURE_WIFI);

        if (err != ESP_OK)
        {
            printf("Error: pcap failed to open\n");
            TERMINAL_VIEW_ADD_TEXT("Error: pcap failed to open\n");
            return 0;
        }
        wifi_manager_start_monitor_mode(wifi_beacon_scan_callback);
    }

    if (strcmp(capturetype, "-raw") == 0)
    {
        printf("Starting raw\npacket capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting raw\npacket capture...\n");
        int err = pcap_file_open("rawscan", PCAP_CAPTURE_WIFI);

        if (err != ESP_OK)
        {
            printf("Error: pcap failed to open\n");
            TERMINAL_VIEW_ADD_TEXT("Error: pcap failed to open\n");
            return 0;
        }
        wifi_manager_start_monitor_mode(wifi_raw_scan_callback);
    }

    if (strcmp(capturetype, "-eapol") == 0)
    {
        printf("Starting EAPOL\npacket capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting EAPOL\npacket capture...\n");
        int err = pcap_file_open("eapolscan", PCAP_CAPTURE_WIFI);

        if (err != ESP_OK)
        {
            printf("Error: pcap failed to open\n");
            TERMINAL_VIEW_ADD_TEXT("Error: pcap failed to open\n");
            return 0;
        }
        wifi_manager_start_monitor_mode(wifi_eapol_scan_callback);
    }

    if (strcmp(capturetype, "-pwn") == 0)
    {
        printf("Starting PWN\npacket capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting PWN\npacket capture...\n");
        int err = pcap_file_open("pwnscan", PCAP_CAPTURE_WIFI);

        if (err != ESP_OK)
        {
            printf("Error: pcap failed to open\n");
            TERMINAL_VIEW_ADD_TEXT("Error: pcap failed to open\n");
            return 0;
        }
        wifi_manager_start_monitor_mode(wifi_pwn_scan_callback);
    }

    if (strcmp(capturetype, "-wps") == 0)
    {
        printf("Starting WPS\npacket capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting WPS\npacket capture...\n");
        int err = pcap_file_open("wpsscan", PCAP_CAPTURE_WIFI);

        should_store_wps = 0;

        if (err != ESP_OK)
        {
            printf("Error: pcap failed to open\n");
            TERMINAL_VIEW_ADD_TEXT("Error: pcap failed to open\n");
            return 0;
        }
        wifi_manager_start_monitor_mode(wifi_wps_detection_callback);
    }

    if (strcmp(capturetype, "-stop") == 0)
    {
        printf("Stopping packet capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Stopping packet capture...\n");
        wifi_manager_stop_monitor_mode();
#ifndef CONFIG_IDF_TARGET_ESP32S2
        ble_stop();
        ble_stop_skimmer_detection();
#endif
        pcap_file_close();
    }
#ifndef CONFIG_IDF_TARGET_ESP32S2
    if (strcmp(capturetype, "-ble") == 0)
    {
        printf("Starting BLE packet capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting BLE packet capture...\n");
        ble_start_capture();
    }

    if (strcmp(capturetype, "-skimmer") == 0)
    {
        printf("Skimmer detection started.\n");
        TERMINAL_VIEW_ADD_TEXT("Skimmer detection started.\n");
        int err = pcap_file_open("skimmer_scan", PCAP_CAPTURE_BLUETOOTH);
        if (err != ESP_OK)
        {
            printf("Warning: PCAP capture failed to start\n");
            TERMINAL_VIEW_ADD_TEXT("Warning: PCAP capture failed to start\n");
        }
        else
        {
            printf("PCAP capture started\nMonitoring devices\n");
            TERMINAL_VIEW_ADD_TEXT("PCAP capture started\nMonitoring devices\n");
        }
        // Start skimmer detection
        ble_start_skimmer_detection();
    }
#endif
return 0;
}
esp_console_cmd_t capture_cmd={
.command = "capture",
.help = "    Description: Start a WiFi Capture (Requires SD Card or Flipper)\n"
    "    Usage: capture [OPTION]\n"
    "    Arguments:\n"
    "        -probe   : Start Capturing Probe Packets\n"
    "        -beacon  : Start Capturing Beacon Packets\n"
    "        -deauth   : Start Capturing Deauth Packets\n"
    "        -raw   :   Start Capturing Raw Packets\n"
    "        -wps   :   Start Capturing WPS Packets and there Auth Type"
    "        -pwn   :   Start Capturing Pwnagotchi Packets"
    "        -stop   : Stops the active capture\n\n",
    .hint = NULL,
    .func = &handle_capture_scan,
};

void stop_portal(int argc, char **argv)
{
    wifi_manager_stop_evil_portal();
    printf("Stopping evil portal...\n");
    TERMINAL_VIEW_ADD_TEXT("Stopping evil portal...\n");
}
esp_console_cmd_t stopportal_cmd={
    .command = "stopportal",
    .help = 
    "    Description: Stop Evil Portal\n"
    "    Usage: stopportal\n\n",
    .hint = NULL,
    .func = &stop_portal,
};

void handle_reboot(int argc, char **argv)
{
    printf("Rebooting system...\n");
    TERMINAL_VIEW_ADD_TEXT("Rebooting system...\n");
    esp_restart();
}
esp_console_cmd_t reboot_cmd={
    .command = "reboot",
    .help = "",
    .hint = NULL,
    .func  =&handle_reboot,
};

void handle_startwd(int argc, char **argv)
{
    bool stop_flag = false;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-s") == 0)
        {
            stop_flag = true;
            break;
        }
    }

    if (stop_flag)
    {
        gps_manager_deinit(&g_gpsManager);
        wifi_manager_stop_monitor_mode();
        printf("Wardriving stopped.\n");
        TERMINAL_VIEW_ADD_TEXT("Wardriving stopped.\n");
    }
    else
    {
        gps_manager_init(&g_gpsManager);
        wifi_manager_start_monitor_mode(wardriving_scan_callback);
        printf("Wardriving started.\n");
        TERMINAL_VIEW_ADD_TEXT("Wardriving started.\n");
    }
}
esp_console_cmd_t startwd_cmd={
    .command = "startwd",
    .help = "",
    .hint = NULL,
    .func=&handle_startwd,
};

void handle_scan_ports(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage:\n");
        printf("scanports local [-C/-A/start_port-end_port]\n");
        printf("scanports [IP] [-C/-A/start_port-end_port]\n");
        return;
    }

    bool is_local = strcmp(argv[1], "local") == 0;
    const char *target_ip = NULL;
    const char *port_arg = NULL;

    // Parse arguments based on whether it's a local scan
    if (is_local)
    {
        if (argc < 3)
        {
            printf("Missing port argument for local scan\n");
            return;
        }
        port_arg = argv[2];
    }
    else
    {
        if (argc < 3)
        {
            printf("Missing port argument for IP scan\n");
            return;
        }
        target_ip = argv[1];
        port_arg = argv[2];
    }

    if (is_local)
    {
        wifi_manager_scan_subnet();
        return;
    }

    host_result_t result;
    if (strcmp(port_arg, "-C") == 0)
    {
        scan_ports_on_host(target_ip, &result);
        if (result.num_open_ports > 0)
        {
            printf("Open ports on %s:\n", target_ip);
            for (int i = 0; i < result.num_open_ports; i++)
            {
                printf("Port %d\n", result.open_ports[i]);
            }
        }
    }
    else
    {
        int start_port, end_port;
        if (strcmp(port_arg, "-A") == 0)
        {
            start_port = 1;
            end_port = 65535;
        }
        else if (sscanf(port_arg, "%d-%d", &start_port, &end_port) != 2 || start_port < 1 ||
                 end_port > 65535 || start_port > end_port)
        {
            printf("Invalid port range\n");
            return;
        }
        scan_ip_port_range(target_ip, start_port, end_port);
    }
}
esp_console_cmd_t scanports_cmd = {
    .command = "scanports",
    .help = "    Description: Scan ports on local subnet or specific IP\n"
"    Usage: scanports local [-C/-A/start_port-end_port]\n"
"           scanports [IP] [-C/-A/start_port-end_port]\n"
"    Arguments:\n"
"        -C  : Scan common ports only\n"
"        -A  : Scan all ports (1-65535)\n"
"        start_port-end_port : Custom port range (e.g. 80-443)\n\n",
.hint = NULL,
.func = &handle_scan_ports,
};

void handle_crash(int argc, char **argv)
{
    int *ptr = NULL;
    *ptr = 42;
}


void handle_capture(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: capture [-probe|-beacon|-deauth|-raw|-ble]\n");
        TERMINAL_VIEW_ADD_TEXT("Usage: capture [-probe|-beacon|-deauth|-raw|-ble]\n");
        return;
    }
#ifndef CONFIG_IDF_TARGET_ESP32S2
    if (strcmp(argv[1], "-ble") == 0)
    {
        printf("Starting BLE packet capture...\n");
        TERMINAL_VIEW_ADD_TEXT("Starting BLE packet capture...\n");
        ble_start_capture();
    }
#endif
}

void handle_gps_info(int argc, char **argv)
{
    bool stop_flag = false;
    static TaskHandle_t gps_info_task_handle = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-s") == 0)
        {
            stop_flag = true;
            break;
        }
    }

    if (stop_flag)
    {
        if (gps_info_task_handle != NULL)
        {
            vTaskDelete(gps_info_task_handle);
            gps_info_task_handle = NULL;
            gps_manager_deinit(&g_gpsManager);
            printf("GPS info display stopped.\n");
            TERMINAL_VIEW_ADD_TEXT("GPS info display stopped.\n");
        }
    }
    else
    {
        if (gps_info_task_handle == NULL)
        {
            gps_manager_init(&g_gpsManager);

            // Wait a brief moment for GPS initialization
            vTaskDelay(pdMS_TO_TICKS(100));

            // Start the info display task
            xTaskCreate(gps_info_display_task, "gps_info", 4096, NULL, 1, &gps_info_task_handle);
            printf("GPS info started.\n");
            TERMINAL_VIEW_ADD_TEXT("GPS info started.\n");
        }
    }
}
esp_console_cmd_t gpsinfo_cmd= {
    .command = "gpsinfo",
    .help = "",
    .hint  = NULL,
    .func=&handle_gps_info,
};

#ifndef CONFIG_IDF_TARGET_ESP32S2
void handle_ble_wardriving(int argc, char **argv)
{
    bool stop_flag = false;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-s") == 0)
        {
            stop_flag = true;
            break;
        }
    }

    if (stop_flag)
    {
        ble_stop();
        gps_manager_deinit(&g_gpsManager);
        if (buffer_offset > 0)
        { // Only flush if there's data in buffer
            csv_flush_buffer_to_file();
        }
        csv_file_close();
        printf("BLE wardriving stopped.\n");
        TERMINAL_VIEW_ADD_TEXT("BLE wardriving stopped.\n");
    }
    else
    {
        if (!g_gpsManager.isinitilized)
        {
            gps_manager_init(&g_gpsManager);
        }

        // Open CSV file for BLE wardriving
        esp_err_t err = csv_file_open("ble_wardriving");
        if (err != ESP_OK)
        {
            printf("Failed to open CSV file for BLE wardriving\n");
            return;
        }

        ble_register_handler(ble_wardriving_callback);
        ble_start_scanning();
        printf("BLE wardriving started.\n");
        TERMINAL_VIEW_ADD_TEXT("BLE wardriving started.\n");
    }
}
#endif

void handle_pineap_detection(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "-s") == 0)
    {
        printf("Stopping PineAP detection...\n");
        TERMINAL_VIEW_ADD_TEXT("Stopping PineAP detection...\n");
        stop_pineap_detection();
        wifi_manager_stop_monitor_mode();
        pcap_file_close();
        return;
    }
    // Open PCAP file for logging detections
    int err = pcap_file_open("pineap_detection", PCAP_CAPTURE_WIFI);
    if (err != ESP_OK)
    {
        printf("Warning: Failed to open PCAP file for logging\n");
        TERMINAL_VIEW_ADD_TEXT("Warning: Failed to open PCAP file for logging\n");
    }

    // Start PineAP detection with channel hopping
    start_pineap_detection();
    wifi_manager_start_monitor_mode(wifi_pineap_detector_callback);

    printf("Monitoring for Pineapples\n");
    TERMINAL_VIEW_ADD_TEXT("Monitoring for Pineapples\n");
}
esp_console_cmd_t pineap_cmd={
    .command = "pineap",
    .help = "",
    .hint = NULL,
    .func=&handle_pineap_detection,
} ;


void handle_apcred(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: apcred <ssid> <password>\n");
        printf("       apcred -r (reset to defaults)\n");
        TERMINAL_VIEW_ADD_TEXT("Usage:\napcred <ssid> <password>\n");
        TERMINAL_VIEW_ADD_TEXT("apcred -r\n");
        return;
    }

    // Check for reset flag
    if (argc == 2 && strcmp(argv[1], "-r") == 0)
    {
        // Set empty strings to trigger default values
        settings_set_ap_ssid(&G_Settings, "");
        settings_set_ap_password(&G_Settings, "");
        settings_save(&G_Settings);
        ap_manager_stop_services();
        esp_err_t err = ap_manager_start_services();
        if (err != ESP_OK)
        {
            printf("Error resetting AP: %s\n", esp_err_to_name(err));
            TERMINAL_VIEW_ADD_TEXT("Error resetting AP:\n%s\n", esp_err_to_name(err));
            return;
        }

        printf("AP credentials reset to defaults (SSID: GhostNet, Password: GhostNet)\n");
        TERMINAL_VIEW_ADD_TEXT("AP reset to defaults:\nSSID: GhostNet\nPSK: GhostNet\n");
        return;
    }

    if (argc != 3)
    {
        printf("Error: Incorrect number of arguments.\n");
        TERMINAL_VIEW_ADD_TEXT("Error: Bad args\n");
        return;
    }

    const char *new_ssid = argv[1];
    const char *new_password = argv[2];

    if (strlen(new_password) < 8)
    {
        printf("Error: Password must be at least 8 characters\n");
        TERMINAL_VIEW_ADD_TEXT("Error: Password must\nbe 8+ chars\n");
        return;
    }

    // immediate AP reconfiguration
    wifi_config_t ap_config = {
        .ap = {
            .ssid_len = strlen(new_ssid),
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK},
    };
    strcpy((char *)ap_config.ap.ssid, new_ssid);
    strcpy((char *)ap_config.ap.password, new_password);

    // Force the new config immediately
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    settings_set_ap_ssid(&G_Settings, new_ssid);
    settings_set_ap_password(&G_Settings, new_password);
    settings_save(&G_Settings);

    const char *saved_ssid = settings_get_ap_ssid(&G_Settings);
    const char *saved_password = settings_get_ap_password(&G_Settings);
    if (strcmp(saved_ssid, new_ssid) != 0 || strcmp(saved_password, new_password) != 0)
    {
        printf("Error: Failed to save AP credentials\n");
        TERMINAL_VIEW_ADD_TEXT("Error: Failed to\nsave credentials\n");
        return;
    }

    ap_manager_stop_services();
    esp_err_t err = ap_manager_start_services();
    if (err != ESP_OK)
    {
        printf("Error restarting AP: %s\n", esp_err_to_name(err));
        TERMINAL_VIEW_ADD_TEXT("Error restart AP:\n%s\n", esp_err_to_name(err));
        return;
    }

    printf("AP credentials updated - SSID: %s, Password: %s\n", saved_ssid, saved_password);
    TERMINAL_VIEW_ADD_TEXT("AP updated:\nSSID: %s\n", saved_ssid);
}
esp_console_cmd_t apcred_cmd = {
    .command = "apcred",
    .help = "",
    .hint = NULL,
    .func = &handle_apcred,
};

void handle_rgb_mode(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: rgbmode <rainbow|police|strobe|off|color>\n");
        TERMINAL_VIEW_ADD_TEXT("Usage: rgbmode <rainbow|police|strobe|off|color>\n");
        return;
    }

    // Cancel any currently running LED effect task.
    if (rgb_effect_task_handle != NULL)
    {
        vTaskDelete(rgb_effect_task_handle);
        rgb_effect_task_handle = NULL;
    }

    // Check for built-in modes first.
    if (strcasecmp(argv[1], "rainbow") == 0)
    {
        xTaskCreate(rainbow_task, "rainbow_effect", 4096, &rgb_manager, 5, &rgb_effect_task_handle);
        printf("Rainbow mode activated\n");
        TERMINAL_VIEW_ADD_TEXT("Rainbow mode activated\n");
    }
    else if (strcasecmp(argv[1], "police") == 0)
    {
        xTaskCreate(police_task, "police_effect", 4096, &rgb_manager, 5, &rgb_effect_task_handle);
        printf("Police mode activated\n");
        TERMINAL_VIEW_ADD_TEXT("Police mode activated\n");
    }
    else if (strcasecmp(argv[1], "strobe") == 0)
    {
        printf("SEIZURE WARNING\nPLEASE EXIT NOW IF\nYOU ARE SENSITIVE\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
        xTaskCreate(strobe_task, "strobe_effect", 4096, &rgb_manager, 5, &rgb_effect_task_handle);
        printf("Strobe mode activated\n");
        TERMINAL_VIEW_ADD_TEXT("Strobe mode activated\n");
    }
    else if (strcasecmp(argv[1], "off") == 0)
    {
        rgb_manager_set_color(&rgb_manager, 0, 0, 0, 0, false);
        led_strip_refresh(rgb_manager.strip);
        printf("RGB disabled\n");
        TERMINAL_VIEW_ADD_TEXT("RGB disabled\n");
    }
    else
    {
        // Otherwise, treat the argument as a color name.
        typedef struct
        {
            const char *name;
            uint8_t r;
            uint8_t g;
            uint8_t b;
        } color_t;
        static const color_t supported_colors[] = {
            {"red", 255, 0, 0},
            {"green", 0, 255, 0},
            {"blue", 0, 0, 255},
            {"yellow", 255, 255, 0},
            {"purple", 128, 0, 128},
            {"cyan", 0, 255, 255},
            {"orange", 255, 165, 0},
            {"white", 255, 255, 255},
            {"pink", 255, 192, 203}};
        const int num_colors = sizeof(supported_colors) / sizeof(supported_colors[0]);
        int found = 0;
        uint8_t r, g, b;
        for (int i = 0; i < num_colors; i++)
        {
            // Use case-insensitive compare.
            if (strcasecmp(argv[1], supported_colors[i].name) == 0)
            {
                r = supported_colors[i].r;
                g = supported_colors[i].g;
                b = supported_colors[i].b;
                found = 1;
                break;
            }
        }
        if (!found)
        {
            printf("Unknown color '%s'. Supported colors: red, green, blue, yellow, purple, cyan, orange, white, pink.\n", argv[1]);
            TERMINAL_VIEW_ADD_TEXT("Unknown color '%s'. Supported colors: red, green, blue, yellow, purple, cyan, orange, white, pink.\n", argv[1]);
            return;
        }
        // Set each LED to the selected static color.
        for (int i = 0; i < rgb_manager.num_leds; i++)
        {
            rgb_manager_set_color(&rgb_manager, i, r, g, b, false);
        }
        led_strip_refresh(rgb_manager.strip);
        printf("Static color mode activated: %s\n", argv[1]);
        TERMINAL_VIEW_ADD_TEXT("Static color mode activated: %s\n", argv[1]);
    }
}
esp_console_cmd_t  rgbmode_cmd = {
    .command = "rgbmode",
    .help = "",
    .hint = NULL,
    .func =& handle_rgb_mode,
};

void handle_setrgb(int argc, char **argv)
{
    if (argc != 4)
    {
        printf("Usage: setrgbpins <red> <green> <blue>\n");
        printf("           (use same value for all pins for single-pin LED strips)\n\n");
        return;
    }

    gpio_num_t red_pin = (gpio_num_t)atoi(argv[1]);
    gpio_num_t green_pin = (gpio_num_t)atoi(argv[2]);
    gpio_num_t blue_pin = (gpio_num_t)atoi(argv[3]);

    // Handle single-pin mode if all values match
    if (red_pin == green_pin && green_pin == blue_pin)
    {
        rgb_manager_deinit(&rgb_manager);
        esp_err_t ret = rgb_manager_init(&rgb_manager, red_pin, 1, // Use single pin mode
                                         LED_PIXEL_FORMAT_GRB, LED_MODEL_WS2812,
                                         GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC); // NC for separate pins
        if (ret == ESP_OK)
        {
            printf("Single-pin RGB configured on GPIO %d!\n", red_pin);
        }
    }
    else
    {
        // Original separate pin logic
        rgb_manager_deinit(&rgb_manager);
        esp_err_t ret = rgb_manager_init(&rgb_manager, GPIO_NUM_NC, 1,
                                         LED_PIXEL_FORMAT_GRB, LED_MODEL_WS2812,
                                         red_pin, green_pin, blue_pin);
        if (ret == ESP_OK)
        {
            printf("RGB pins updated successfully!\n");
        }
    }
}
esp_console_cmd_t setrgb_cmd = {
    .command= "setrgb",
    .help = "",
    .hint = NULL,
    .func = &handle_setrgb,

};

esp_console_cmd_t powerprinter_cmd = {
    .command = "powerprinter",
    .help = "    Description: Print Custom Text to a Printer on your LAN "
            "(Requires You to Run Connect First)\n"
            "    Usage: powerprinter <Printer IP> <Text> <FontSize> <alignment>\n"
            "    aligment options: CM = Center Middle, TL = Top Left, TR = Top "
            "Right, BR = Bottom Right, BL = Bottom Left\n\n",
    .hint = NULL,
    .func = &handle_printer_command};
void register_commands()
{
    esp_console_register_help_command();
    esp_console_cmd_register(&stop_wifi_scan_cmd);
    esp_console_cmd_register(&start_wifi_scan_cmd);
    esp_console_cmd_register(&wifi_sta_scan_cmd);
    esp_console_cmd_register(&scanlocal_cmd);
    esp_console_cmd_register(&attack_cmd);
    esp_console_cmd_register(&handle_list_cmd);
    esp_console_cmd_register(&beaconspam_cmd );
    esp_console_cmd_register(&stopspam_cmd);
    esp_console_cmd_register(&stopdeauth_cmd);
    esp_console_cmd_register(&select_cmd);
    esp_console_cmd_register(&capture_cmd);
    esp_console_cmd_register(&startportal_cmd);
    esp_console_cmd_register(&stopportal_cmd );
    esp_console_cmd_register(&connect_cmd );
    esp_console_cmd_register(&dialconnect_cmd) ;
    esp_console_cmd_register(&powerprinter_cmd);
    esp_console_cmd_register(&tplinktest_cmd);
    esp_console_cmd_register(&stop_cmd);
    esp_console_cmd_register(&reboot_cmd);
    esp_console_cmd_register(&startwd_cmd);
    esp_console_cmd_register(&gpsinfo_cmd);
    esp_console_cmd_register(&scanports_cmd);
// #ifndef CONFIG_IDF_TARGET_ESP32S2
//     register_command("blescan", handle_ble_scan_cmd);
//     register_command("blewardriving", handle_ble_wardriving);
// #endif
// #ifdef DEBUG
//     register_command("crash", handle_crash); // For Debugging
// #endif
    esp_console_cmd_register(&pineap_cmd);
    esp_console_cmd_register(&apcred_cmd);
    esp_console_cmd_register(&rgbmode_cmd);
    esp_console_cmd_register(&setrgb_cmd);
    printf("Registered Commands\n");
    TERMINAL_VIEW_ADD_TEXT("Registered Commands\n");
}
