#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include <stdatomic.h>
#include <cJSON.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "dv8_mqtt.h"

static const char *TAG = "mqtt_example";



static void save_value_float(cJSON *json, _Atomic(float) *parameter, char *parameter_name)
{
    cJSON *temp = cJSON_GetObjectItem(json,parameter_name);
    if (temp) {
	atomic_store(parameter,cJSON_GetNumberValue(temp));
	//ESP_LOGI(TAG,"RECIEVE: %s: %f",parameter_name,parameter);
    }
}

static void save_value_int(cJSON *json, _Atomic(int) *parameter, char *parameter_name)                                                              {
    cJSON *temp = cJSON_GetObjectItem(json,parameter_name);
    if (temp) {
	atomic_store(parameter,cJSON_GetNumberValue(temp));
	//ESP_LOGI(TAG,"RECIEVE: %s: %d",parameter_name,parameter);
    }
}

// static void log_error_if_nonzero(const char *message, int error_code)
// {
//     if (error_code != 0) {
//         ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
//     }
// }

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    switch ((esp_mqtt_event_id_t)event_id) {
	case MQTT_EVENT_CONNECTED:
	    esp_mqtt_client_subscribe(client, "/robot/control/cmd_vel", 0);
	    esp_mqtt_client_subscribe(client, "/robot/state/battery_percentage", 0);
	    esp_mqtt_client_subscribe(client, "/robot/state/battery_is_charging", 0);
	    esp_mqtt_client_subscribe(client, "/robot/state/e_stop", 0);
	    esp_mqtt_client_subscribe(client, "/robot/state/handbrake", 0);
	    esp_mqtt_client_subscribe(client, "/robot/state/direct_status", 0);
	    esp_mqtt_client_subscribe(client, "/robot/state/robot_mode", 0);
	    esp_mqtt_client_subscribe(client, "/robot/control/brush_speed", 0);
		esp_mqtt_client_subscribe(client, "/robot/state/safety_mode", 0);
	    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
	    break;
	case MQTT_EVENT_DISCONNECTED:
	    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
	    break;
	case MQTT_EVENT_DATA: 
	    {
		int topic_len = event->topic_len;
		char topic[topic_len + 1];
		int data_len = event->data_len;
		char data[data_len + 1];

		if (topic_len == 0 || data_len == 0) {
		    break;
		}

		strncpy(topic, event->topic, topic_len);
		topic[topic_len] = '\0';
		strncpy(data, event->data, data_len);
		data[data_len] = '\0';	

		ESP_LOGI(TAG,"ON MQTT TOPIC: %s", topic);

		cJSON *json = cJSON_Parse(data);

		if (json == NULL) {
		    ESP_LOGI(TAG,"RECIEVE ERROR DATA: %s", data);
		    cJSON_Delete(json);
		    break;		    
		}

		if (strcmp(topic,"/robot/control/cmd_vel") == 0) {
		    save_value_float(json,&linear_x,"linear_x");
		    save_value_float(json,&angular_z,"angular_z");
		} else if (strcmp(topic,"/robot/state/battery_percentage") == 0) {
		    save_value_float(json,&battery_percentage,"battery_percentage");    
		} else if (strcmp(topic,"/robot/state/battery_is_charging") == 0) {
		    save_value_int(json,&battery_is_charging,"battery_is_charging");
		} else if (strcmp(topic,"/robot/state/e_stop") == 0) {
		    save_value_int(json,&e_stop,"e_stop");    
		} else if (strcmp(topic,"/robot/state/handbrake") == 0) {
		    save_value_int(json,&handbrake,"handbrake");
		} else if (strcmp(topic,"/robot/state/direct_status") == 0) {
		    save_value_int(json,&direct_status,"direct_status");
		} else if (strcmp(topic,"/robot/state/robot_mode") == 0) {
		    save_value_int(json,&robot_mode,"robot_mode");
		} else if (strcmp(topic,"/robot/control/brush_speed") == 0) {
		    save_value_int(json,&brush_speed,"brush_speed");
		} else if (strcmp(topic,"/robot/state/safety_mode") == 0) {
			save_value_int(json,&safety_mode,"safety_mode");
		}
		cJSON_Delete(json);
		break;
	    }
	case MQTT_EVENT_ERROR:
	    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
	    break;
	default:
	    break;
    }
}


// static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
// {
//     ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
//     esp_mqtt_event_handle_t event = event_data;
//     esp_mqtt_client_handle_t client = event->client;
//     int msg_id;
//     switch ((esp_mqtt_event_id_t)event_id) {
//     case MQTT_EVENT_CONNECTED:
//         ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
//         msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
//         ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

//         msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
//         ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

//         msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
//         ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

//         msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
//         ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
//         break;
//     case MQTT_EVENT_DISCONNECTED:
//         ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
//         break;

//     case MQTT_EVENT_SUBSCRIBED:
//         ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
//         msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
//         ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
//         break;
//     case MQTT_EVENT_UNSUBSCRIBED:
//         ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
//         break;
//     case MQTT_EVENT_PUBLISHED:
//         ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
//         break;
//     case MQTT_EVENT_DATA:
//         ESP_LOGI(TAG, "MQTT_EVENT_DATA");
//         printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
//         printf("DATA=%.*s\r\n", event->data_len, event->data);
//         break;
//     case MQTT_EVENT_ERROR:
//         ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
//         if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
//             log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
//             log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
//             log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
//             ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

//         }
//         break;
//     default:
//         ESP_LOGI(TAG, "Other event id:%d", event->event_id);
//         break;
//     }
// }

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
    };
    ESP_LOGI(TAG, "Connecting to broker at IP: %s", CONFIG_BROKER_URL);
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.broker.address.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.broker.address.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void mqtt_module_start(void)
{
    ESP_LOGI(TAG, "[APP] Starting MQTT Main");
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_esp", ESP_LOG_ERROR);

    //ESP_ERROR_CHECK(nvs_flash_init());
    //ESP_ERROR_CHECK(esp_netif_init());
    //ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    //ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();
}
