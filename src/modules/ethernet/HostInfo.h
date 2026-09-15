#ifndef HOST_INFO_H
#define HOST_INFO_H

#include "ESPNetifEthernetClient.h"
#include "modules/wifi/scan_hosts.h"
#include <map>
class HostInfo {
private:
    WiFiClient *wifi_client = nullptr;
#if !defined(LITE_VERSION)
    ESPNetifEthernetClient *eth_client = nullptr;
#endif
    void setup(const Host &host);
    /*
    std::map<int, const char *> portServices = {
        //  hmm
        //  yea this was not good so its gone now :]
        {19,    "CHARGEN"                                                          },
        {20,    "FTP Data, FTP-DATA"                                               },
        {21,    "FTP Control, SFTP, FTP"                                           },
        {22,    "SSH, SFTP, SCP"                                                   },
        {23,    "Telnet"                                                           },
        {25,    "SMTP, SMTP (Submission)"                                          },
        {42,    "WINS Replication, nameserver"                                     },
        {53,    "DNS, DNS-TCP, DNS-UDP"                                            },
        {67,    "DHCP Server, BOOTP Server"                                        },
        {68,    "DHCP Client, BOOTP Client"                                        },
        {69,    "TFTP, TFTP-UDP"                                                   },
        {80,    "HTTP, HTTP-ALT, Web"                                              },
        {88,    "Kerberos, Kerberos5"                                              },
        {110,   "POP3, Post Office Protocol"                                       },
        {111,   "RPCbind, Portmapper"                                              },
        {113,   "Ident, Auth, Authentication"                                      },
        {119,   "NNTP, Network News Transfer Protocol"                             },
        {123,   "NTP, Network Time Protocol"                                       },
        {135,   "Microsoft RPC, EPMAP, DCE endpoint resolution"                    },
        {137,   "NetBIOS Name Service"                                             },
        {139,   "NetBIOS Session Service, SMB"                                     },
        {143,   "IMAP, IMAP4"                                                      },
        {161,   "SNMP"                                                             },
        {162,   "SNMP Trap, SNMPTRAP"                                              },
        {179,   "BGP, Border Gateway Protocol"                                     },
        {194,   "IRC, Internet Relay Chat"                                         },
        {389,   "LDAP, Directory Services"                                         },
        {427,   "SLP, Service Location Protocol"                                   },
        {443,   "HTTPS, SSL/TLS, HTTP over TLS/SSL"                                },
        {445,   "Microsoft-DS, SMB over TCP"                                       },
        {464,   "Kerberos Change/Set password"                                     },
        {465,   "SMTPS, SMTP over SSL"                                             },
        {500,   "ISAKMP, Internet Security Association and Key Management Protocol"},
        {514,   "Syslog, Remote Shell"                                             },
        {515,   "LPD, Line Printer Daemon"                                         },
        {520,   "RIP, Routing Information Protocol"                                },
        {554,   "RTSP, Real Time Streaming Protocol"                               },
        {587,   "SMTP (Submission), Message Submission"                            },
        {631,   "IPP, Internet Printing Protocol, CUPS"                            },
        {636,   "LDAPS, LDAP over SSL"                                             },
        {873,   "rsync, Remote Sync"                                               },
        {902,   "VMware Server, VMware ESXi"                                       },
        {989,   "FTPS Data, FTP over SSL Data"                                     },
        {990,   "FTPS Control, FTP over SSL Control"                               },
        {993,   "IMAPS, IMAP over SSL"                                             },
        {995,   "POP3S, POP3 over SSL"                                             },
        {1024,  "Reserved, First Available Dynamic Port"                           },
        {1025,  "NFS-or-IIS, Microsoft RPC"                                        },
        {1080,  "SOCKS Proxy, SOCKS"                                               },
        {1194,  "OpenVPN, VPN"                                                     },
        {1337,  "WASTE, WASTE Encrypted File Sharing"                              },
        {1352,  "Lotus Notes, IBM Notes"                                           },
        {1433,  "MSSQL, Microsoft SQL Server"                                      },
        {1434,  "MSSQL Monitor, SQL Server Browser Service"                        },
        {1512,  "WINS, Windows Internet Name Service"                              },
        {1521,  "Oracle DB, Oracle Database, TNS Listener"                         },
        {1604,  "Citrix, ICA"                                                      },
        {1701,  "L2TP, Layer 2 Tunneling Protocol"                                 },
        {1720,  "H.323, VoIP"                                                      },
        {1723,  "PPTP, VPN"                                                        },
        {1812,  "RADIUS Authentication"                                            },
        {1813,  "RADIUS Accounting"                                                },
        {1883,  "MQTT, Message Queuing Telemetry Transport"                        },
        {1900,  "SSDP, UPnP Discovery"                                             },
        {2049,  "NFS, Network File System"                                         },
        {2082,  "cPanel, HTTP Web Management"                                      },
        {2083,  "cPanel, Secure Web Management"                                    },
        {2086,  "WHM, Web Host Manager HTTP"                                       },
        {2087,  "WHM, Web Host Manager HTTPS"                                      },
        {2095,  "Webmail, cPanel Webmail"                                          },
        {2181,  "Zookeeper, Apache ZooKeeper"                                      },
        {2222,  "SSH Alt, DirectAdmin"                                             },
        {2375,  "Docker, Docker REST API"                                          },
        {2376,  "Docker TLS, Docker Secure"                                        },
        {2379,  "etcd Client, etcd API"                                            },
        {2380,  "etcd Server, etcd Peer"                                           },
        {3128,  "Squid Proxy, HTTP Proxy"                                          },
        {3306,  "MySQL, MariaDB"                                                   },
        {3389,  "RDP, Remote Desktop Protocol, Terminal Services"                  },
        {3690,  "SVN, Subversion"                                                  },
        {4000,  "Remote Login, Remotely Anywhere"                                  },
        {4500,  "IPSec NAT-T, VPN"                                                 },
        {4789,  "VXLAN, Virtual Extensible LAN"                                    },
        {5000,  "UPnP, Docker Registry"                                            },
        {5060,  "SIP, Session Initiation Protocol, VoIP"                           },
        {5061,  "SIP over TLS, Secure SIP"                                         },
        {5222,  "XMPP, Jabber"                                                     },
        {5353,  "mDNS, Multicast DNS, Bonjour"                                     },
        {5432,  "PostgreSQL, Postgres"                                             },
        {5671,  "AMQP over TLS, Secure AMQP"                                       },
        {5672,  "AMQP, RabbitMQ, Advanced Message Queuing Protocol"                },
        {5900,  "VNC, Virtual Network Computing"                                   },
        {5985,  "WinRM HTTP, Windows Remote Management"                            },
        {5986,  "WinRM HTTPS, Windows Remote Management Secure"                    },
        {6379,  "Redis, Redis Database"                                            },
        {6443,  "Kubernetes API, k8s API Server"                                   },
        {6514,  "Syslog over TLS"                                                  },
        {6667,  "IRC, Internet Relay Chat"                                         },
        {7001,  "WebLogic, Oracle WebLogic"                                        },
        {7077,  "Apache Spark"                                                     },
        {8000,  "HTTP Alt, HTTP Alternative"                                       },
        {8080,  "HTTP Proxy, Web Proxy, Alternative HTTP"                          },
        {8081,  "HTTP Proxy Alt, Web Proxy Alternative"                            },
        {8443,  "HTTPS Alt, HTTPS Alternative"                                     },
        {8444,  "Chia, Chia Blockchain"                                            },
        {8888,  "HTTP Alt, Alternative Web Server"                                 },
        {9000,  "SonarQube, Portainer"                                             },
        {9042,  "Cassandra, Apache Cassandra"                                      },
        {9090,  "Prometheus, Web Proxy"                                            },
        {9100,  "Raw Printing (JetDirect)"                                         },
        {9200,  "Elasticsearch, Elastic Search HTTP"                               },
        {9300,  "Elasticsearch Nodes, Elastic Search Transport"                    },
        {9418,  "Git, Git Protocol"                                                },
        {10000, "Webmin, Web-based System Administration"                          },
        {11211, "Memcached, Memory Cache Daemon"                                   },
        {15672, "RabbitMQ Management Console"                                      },
        {27017, "MongoDB, MongoDB Wire Protocol"                                   },
        {32768, "RPC, First Available Dynamic Port"                                },
        {49152, "Windows RPC"                                                      },
        {49153, "Windows RPC"                                                      },
        {49154, "Windows RPC"                                                      },
        {49155, "Windows RPC"                                                      },
        {49156, "Windows RPC"                                                      },
        {49157, "Windows RPC"                                                      }
    }; */
    void client_stop();
    void client_connect(IPAddress ip, int port, int timeout_ms = 3000);
    bool client_connected();
    int sockfd = -1;

public:
    HostInfo();
    HostInfo(const Host &host, bool wifi);
    ~HostInfo();
};

#endif
