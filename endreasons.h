#ifndef __REASONS_H__
#define __REASONS_H__

//Call ending cause
#define CM_CALL_END_OFFLINE   0  // The board is offline.
#define CM_CALL_END_NO_SRV   21  //Board is out of service.
#define CM_CALL_END_FADE   22 //Call is ended normally.
#define CM_CALL_END_INTERCEPT   23     //Call is interrupted by BS.
#define CM_CALL_END_REORDER   24 //BS record is received during a call.
#define CM_CALL_END_REL_NORMAL   25 //BS releases a call.
#define CM_CALL_END_REL_SO_REJ   26 //BS rejects the current SO service.
#define CM_CALL_END_INCOM_CALL   27//There is incoming BS call.
#define CM_CALL_END_ALERT_STOP   28 //received alert stop from BS.
#define CM_CALL_END_CLIENT_END   29 //Call is ended normally by the client end.
#define CM_CALL_END_ACTIVATION   30 //received end activation - OTASP call.
#define CM_CALL_END_MC_ABORT   31 //MC ends call initiation or call.
#define CM_CALL_END_RUIM_NOT_PRESENT   34 //RUIM is not available.
#define CM_CALL_END_NDSS_FAIL   99 //NDSS error.
#define CM_CALL_END_LL_CAUSE   100 //rxd a reason from lower layer,look in cc_cause
#define CM_CALL_END_CONF_FAILED   101 //After a MS initiates a call, the network fails to respond.
#define CM_CALL_END_INCOM_REJ   102 //MS rejects an incoming call.
#define CM_CALL_END_SETUP_REJ   103 //A call is rejected during the put-through process.
#define CM_CALL_END_NETWORK_END   104  //The release is from the network. For details, check
#define CM_CALL_END_NO_FUNDS   105 //The phone fee is used up.
#define CM_CALL_END_NO_GW_SRV   106 //The MS is out of the service area.

//Call control cause
#define UNASSIGNED_CAUSE			1
#define NO_ROUTE_TO_DEST			3
#define CHANNEL_UNACCEPTABLE			6
#define OPERATOR_DETERMINED_BARRING                      8
#define NORMAL_CALL_CLEARING                             16
#define USER_BUSY                                        17
#define NO_USER_RESPONDING                              18
#define USER_ALERTING_NO_ANSWER                         19
#define CALL_REJECTED                                   21
#define NUMBER_CHANGED                                  22
#define NON_SELECTED_USER_CLEARING                      26
#define DESTINATION_OUT_OF_ORDER                                                  27
#define INVALID_NUMBER_FORMAT                                                     28
#define FACILITY_REJECTED                                                         29
#define RESPONSE_TO_STATUS_ENQUIRY                                                30
#define NORMAL_UNSPECIFIED                                                        31
#define NO_CIRCUIT_CHANNEL_AVAILABLE                                              34
#define NETWORK_OUT_OF_ORDER                                                      38
#define TEMPORARY_FAILURE                                                         41
#define SWITCHING_EQUIPMENT_CONGESTION                                            42
#define ACCESS_INFORMATION_DISCARDED                                              43
#define REQUESTED_CIRCUIT_CHANNEL_NOT_AVAILABLE 44
#define RESOURCES_UNAVAILABLE_UNSPECIFIED                                         47
#define QUALITY_OF_SERVICE_UNAVAILABLE                                            49
#define REQUESTED_FACILITY_NOT_SUBSCRIBED                                         50
#define INCOMING_CALL_BARRED_WITHIN_CUG                                           55
#define BEARER_CAPABILITY_NOT_AUTHORISED                                          57
#define BEARER_CAPABILITY_NOT_PRESENTLY_AVAILABLE 58
#define SERVICE_OR_OPTION_NOT_AVAILABLE                                            63
#define BEARER_SERVICE_NOT_IMPLEMENTED                                             65
#define ACM_GEQ_ACMMAX                                                            68
#define REQUESTED_FACILITY_NOT_IMPLEMENTED                                        69
#define ONLY_RESTRICTED_DIGITAL_INFO_BC_AVAILABLE 70
#define SERVICE_OR_OPTION_NOT_IMPLEMENTED                                         79
#define INVALID_TRANSACTION_ID_VALUE                                              81
#define USER_NOT_MEMBER_OF_CUG                                                    87
#define INCOMPATIBLE_DESTINATION                                                  88
#define INVALID_TRANSIT_NETWORK_SELECTION                                         91
#define SEMANTICALLY_INCORRECT_MESSAGE                                             95
#define INVALID_MANDATORY_INFORMATION                                              96
#define MESSAGE_TYPE_NON_EXISTENT                                                 97
#define MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROT_STATE 98
#define IE_NON_EXISTENT_OR_NOT_IMPLEMENTED                                         99
#define CONDITIONAL_IE_ERROR 100
#define MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE 101
#define RECOVERY_ON_TIMER_EXPIRY                                                102
#define PROTOCOL_ERROR_UNSPECIFIED                                              111
#define INTERWORKING_UNSPECIFIED                                                127
#define REJ_UNSPECIFIED                                                         160
#define AS_REJ_RR_REL_IND                                                       161
#define AS_REJ_RR_RANDOM_ACCESS_FAILURE                                         162
#define AS_REJ_RRC_REL_IND                                                      163
#define AS_REJ_RRC_CLOSE_SESSION_IND                                            164
#define AS_REJ_RRC_OPEN_SESSION_FAILURE                                          165
#define AS_REJ_LOW_LEVEL_FAIL                                                    166
#define AS_REJ_LOW_LEVEL_FAIL_REDIAL_NOT_ALLOWD 167
#define MM_REJ_INVALID_SIM                                                       168
#define MM_REJ_NO_SERVICE                                                        169
#define MM_REJ_TIMER_T3230_EXP                                                   170
#define MM_REJ_NO_CELL_AVAILABLE                                                 171
#define MM_REJ_WRONG_STATE                                                       172
#define MM_REJ_ACCESS_CLASS_BLOCKED                                              173
#define ABORT_MSG_RECEIVED                                                       174
#define OTHER_CAUSE                                                              175
#define CNM_REJ_TIMER_T303_EXP                                        176
#define CNM_REJ_NO_RESOURCES                                          177
#define CNM_MM_REL_PENDING                                            178
#define CNM_INVALID_USER_DATA                                         179



//Datacard reasons
#define DATACARD_NORMAL 0
#define DATACARD_INCOMPLETE 1
#define DATACARD_NOROUTE 2
#define DATACARD_NOCONN 3
#define DATACARD_NOMEDIA 4
#define DATACARD_NOCALL 5
#define DATACARD_BUSY 6
#define DATACARD_NOANSWER 7
#define DATACARD_REJECTED 8
#define DATACARD_FORBIDDEN 9
#define DATACARD_OFFLINE 10
#define DATACARD_CONGESTION 11
#define DATACARD_FAILURE 12

#endif

/* vi: set ts=8 sw=4 sts=4 noet: */

