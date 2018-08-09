/*
Authors:

150101037: Mukkoti Anil Kumar 
150101045: Patoliya Meetkumar 
150101072: Shubham Singhal


// Network topology
//
//  n0                                   n4
//     \ 80 Mb/s, 20ms 					/
//      \          30Mb/s, 100ms      /
//       n2--------------------------n3
//      /							   \
//     / 80 Mb/s, 20ms                   \	
//   n1 								 n5

*/

#include <string>
#include <fstream>
#include <cstdlib>
#include <map>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/gnuplot.h"

typedef uint32_t uint;

using namespace ns3;

#define ERROR 0.000001

NS_LOG_COMPONENT_DEFINE ("final1");

std::map<std::string, double> mapBytesReceivedIPV4, mapMaxThroughput;
double printGap = 0;

int main (int argc, char *argv[])
{

    uint32_t maxBytes = 0;
	uint32_t port;
	uint32_t packetsize = 1024;
	uint32_t run_time = 1;
	uint32_t for_loop = 1;

	bool simultaneously = false;
    std::string prot = "TcpHighSpeed";
	CommandLine cmd;
    cmd.AddValue ("maxBytes", "Total number of bytes for application to send", maxBytes);
    cmd.AddValue ("packetsize", "Total number of bytes for application to send", packetsize);
    cmd.AddValue ("prot", "Transport protocol to use:TcpHighSpeed, TcpVegas, TcpScalable", prot);
    cmd.AddValue ("for_loop", "no of for loop runs", for_loop);
    cmd.AddValue ("run_time", "run_time in factor of 5", run_time);
    cmd.AddValue ("simultaneously", "run_time in factor of 5", simultaneously);

    cmd.Parse (argc, argv);


    if (prot.compare ("TcpScalable") == 0)
    {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpScalable::GetTypeId ()));
    }
    else if (prot.compare ("TcpVegas") == 0)
    {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
    }
    else if (prot.compare ("TcpHighSpeed") == 0)
    {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpHighSpeed::GetTypeId ()));
    }
    else
    {
        Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpVegas::GetTypeId ()));
    }

	// Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (2100));
	// Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("44kb/s"));

	//Dataset for gnuplot 

    Gnuplot2dDataset dataset_udp;
	Gnuplot2dDataset dataset_tcp;
    Gnuplot2dDataset dataset_udp_delay;
	Gnuplot2dDataset dataset_tcp_delay;

	for(uint i=0;i<for_loop; ++i)
	{		
		uint32_t udpPacketSize= packetsize+100*i;  
		uint32_t tcpPacketSize= udpPacketSize;  

		//create a container for 6 nodes
		NodeContainer c;
		c.Create (6);
		NodeContainer n0n2 = NodeContainer (c.Get (0), c.Get (2));
		NodeContainer n1n2 = NodeContainer (c.Get (1), c.Get (2));
		NodeContainer n2n3 = NodeContainer (c.Get (2), c.Get (3));
		NodeContainer n3n4 = NodeContainer (c.Get (3), c.Get (4));
		NodeContainer n3n5 = NodeContainer (c.Get (3), c.Get (5));


		// installing internet stack in all nodes
		InternetStackHelper internet;
		internet.Install (c);


		uint32_t queueSizeHR = (80000*20)/udpPacketSize;
		uint32_t queueSizeRR = (30000*100)/udpPacketSize;


		//point to point helper is used to create p2p links between nodes
		PointToPointHelper p2p;
		
		//router to host links
		p2p.SetDeviceAttribute ("DataRate", StringValue ("80Mbps"));
		p2p.SetChannelAttribute ("Delay", StringValue ("20ms"));
	    // p2p.SetQueue ("ns3::DropTailQueue");
	    p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue(queueSizeHR));

		NetDeviceContainer d0d2 = p2p.Install (n0n2);
		NetDeviceContainer d1d2 = p2p.Install (n1n2);
		NetDeviceContainer d3d4 = p2p.Install (n3n4);
		NetDeviceContainer d3d5 = p2p.Install (n3n5);

		//router to router link
		p2p.SetDeviceAttribute ("DataRate", StringValue ("30Mbps"));
		p2p.SetChannelAttribute ("Delay", StringValue ("100ms"));
	    p2p.SetQueue ("ns3::DropTailQueue", "MaxPackets", UintegerValue(queueSizeRR));
		NetDeviceContainer d2d3 = p2p.Install (n2n3);

		// //error model
		Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
	    em->SetAttribute ("ErrorRate", DoubleValue (ERROR));
	    d3d4.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
	    d3d5.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

		//assigning IP to the netdevice containers having 2 nodes each
		Ipv4AddressHelper ipv4;
		ipv4.SetBase ("10.1.0.0", "255.255.255.0");
		Ipv4InterfaceContainer i0i2 = ipv4.Assign (d0d2);
		
		ipv4.SetBase ("10.1.1.0", "255.255.255.0");
		Ipv4InterfaceContainer i1i2 = ipv4.Assign (d1d2);

		ipv4.SetBase ("10.1.3.0", "255.255.255.0");
		Ipv4InterfaceContainer i2i3 = ipv4.Assign (d2d3);

		ipv4.SetBase ("10.1.4.0", "255.255.255.0");
		Ipv4InterfaceContainer i3i4 = ipv4.Assign (d3d4);

		ipv4.SetBase ("10.1.5.0", "255.255.255.0");
		Ipv4InterfaceContainer i3i5 = ipv4.Assign (d3d5);

		
		//routuing tables 
		Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

		//printing ips
		std::cout << "Assigned IPs to Receivers:" << std::endl;
		std::cout<<i3i4.GetAddress(0)<<std::endl;
		std::cout<<i3i4.GetAddress(1)<<std::endl;
		std::cout<<i3i5.GetAddress(0)<<std::endl;
		std::cout<<i3i5.GetAddress(1)<<std::endl;
		std::cout << "Assigned IPs to Senders:" << std::endl;
		std::cout<< i0i2.GetAddress(0)<<std::endl;
		std::cout<< i0i2.GetAddress(1)<<std::endl;
		std::cout<< i1i2.GetAddress(0)<<std::endl;
		std::cout<< i1i2.GetAddress(1)<<std::endl;
		std::cout << "Assigned IPs to Router:" << std::endl;
		std::cout<< i2i3.GetAddress(0)<<std::endl;
		std::cout<< i2i3.GetAddress(1)<<std::endl;
		

		//printing routing tables for the all the nodes in the container
		Ipv4GlobalRoutingHelper g;
		Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("routing.routes", std::ios::out);
		g.PrintRoutingTableAllAt (Seconds (2), routingStream);

		/*
			**************************		
				CBR traffic on UDP
			**************************
		*/
		port = 9;  

		// on off helper is for CBR traffic, we tell INET socket address here that receiver is receiver 1
		OnOffHelper onoff ("ns3::UdpSocketFactory", Address (InetSocketAddress (i3i4.GetAddress (1), port)));
		// onoff.SetConstantRate (DataRate ("10000kb/s"),udpPacketSize);
	    onoff.SetAttribute ("PacketSize", UintegerValue (udpPacketSize));

		
		//install the on off app on sender 1 and run for 1-5 seconds
		ApplicationContainer udp_apps_s = onoff.Install (n0n2.Get (0));
		
		//runtime =  (total time of simulation in multiple of 10 for given packet size) 
		//i = for loop counter(packet size is increasing after every loop iteration)
		if(simultaneously==false)
		{
			udp_apps_s.Start (Seconds ( (0.0+(10*i))*run_time  ) );
			udp_apps_s.Stop (Seconds ((5.0+(10* i))*run_time) );			
		}
		else
		{
			udp_apps_s.Start (Seconds ( (0.0+(10*i))*run_time  ) );
			udp_apps_s.Stop (Seconds ((10.0+(10*i))*run_time) );
		}

		// Create a packet sink to receive these packets from any ip address. 
		PacketSinkHelper sink_udp ("ns3::UdpSocketFactory",Address (InetSocketAddress (Ipv4Address::GetAny (), port)));

		// install the reciver at reciever 1 
		ApplicationContainer udp_apps_d = sink_udp.Install (n3n4.Get (1));

		if(simultaneously==false)
		{
			udp_apps_d.Start (Seconds ((0.0+(10*i))*run_time) );
			udp_apps_d.Stop (Seconds ((5.0+(10*i))*run_time) );			
		}
		else
		{
			udp_apps_d.Start (Seconds ((0.0+(10*i))*run_time) );
			udp_apps_d.Stop (Seconds ((10.0+(10*i))*run_time) );			
		}


		/*
			**************************		
				FTP traffic on TCP
			**************************
		*/

		// Create a BulkSendApplication and install it on 2nd reciever
	    port = 12344;
	    BulkSendHelper source ("ns3::TcpSocketFactory",InetSocketAddress (i3i5.GetAddress (1), port));
	    // Set the amount of data to send in bytes.  Zero is unlimited.
	    source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
		source.SetAttribute ("SendSize", UintegerValue (tcpPacketSize));
	    ApplicationContainer tcp_apps_s = source.Install (n1n2.Get (0));
	    
	    if(simultaneously==false)
		{
			tcp_apps_s.Start (Seconds ((5.0+(10*i))*run_time) );
	    	tcp_apps_s.Stop (Seconds ((10.0+(10*i))*run_time) );
		}
		else
		{
			tcp_apps_s.Start (Seconds ((0.0+(10*i))*run_time) );
	    	tcp_apps_s.Stop (Seconds ((10.0+(10*i))*run_time) );	
		}

	    
	    // Create a PacketSinkApplication and install it on node 1
	    PacketSinkHelper sink_tcp ("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (), port));
	    ApplicationContainer tcp_apps_d = sink_tcp.Install (n3n5.Get (1));

	    if(simultaneously==false)
		{
			tcp_apps_d.Start (Seconds ((5.0+(10*i))*run_time) );
	    	tcp_apps_d.Stop (Seconds ((10.0+(10*i))*run_time) );
		}
		else
		{
			tcp_apps_d.Start (Seconds ((0.0+(10*i))*run_time) );
	    	tcp_apps_d.Stop (Seconds ((10.0+(10*i))*run_time) );	
		}
	    // tcpSink = DynamicCast<PacketSink> (sinkApps.Get (0));


		/*
			**************************		
				LOGGING of PARAMETERS
			**************************
		*/

		Ptr<FlowMonitor> flowmon;
		FlowMonitorHelper flowmonHelper;
		flowmon = flowmonHelper.InstallAll();
		Simulator::Stop(Seconds((10+(10*i))*run_time) );
		Simulator::Run();
		flowmon->CheckForLostPackets();

		Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
		std::map<FlowId, FlowMonitor::FlowStats> stats = flowmon->GetFlowStats();
		
		double throughput_udp;
		double throughput_tcp;
		double delay_udp;
		double delay_tcp;
		
		for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) 
		{
			Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
			std::cout<<"asdfasdf\n";
			std::cout<<t.sourceAddress<<"\n";
			std::cout<<t.destinationAddress<<"\n";

			if(t.sourceAddress == "10.1.0.1") {
				throughput_udp = i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstRxPacket.GetSeconds ()) / 1000000;
				delay_udp = i->second.delaySum.GetSeconds()/(i->second.rxPackets) ;

				dataset_udp.Add (udpPacketSize,throughput_udp);
				dataset_udp_delay.Add (udpPacketSize,delay_udp);

				std::cout << "UDP Flow over CBR " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
				std::cout << "Tx Packets: " << i->second.txPackets << "\n";
				std::cout << "Tx Bytes:" << i->second.txBytes << "\n";
				std::cout << "Rx Packets: " << i->second.rxPackets << "\n";
				std::cout << "Rx Bytes:" << i->second.rxBytes << "\n";
				std::cout << "Net Packet Lost: " << i->second.lostPackets << "\n";
				std::cout << "Lost due to droppackets: " << i->second.packetsDropped.size() << "\n";
				std::cout << "Max throughput: " << mapMaxThroughput["/NodeList/0/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
				std::cout << "Max throughput: " << mapMaxThroughput["/NodeList/1/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
				std::cout << "Max throughput: " << mapMaxThroughput["/NodeList/2/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
				std::cout << "Max throughput: " << mapMaxThroughput["/NodeList/3/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
				std::cout << "Max throughput: " << mapMaxThroughput["/NodeList/4/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
				std::cout << "Max throughput: " << mapMaxThroughput["/NodeList/5/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
				std::cout << "Delay: " << i->second.delaySum.GetSeconds() << std::endl;
				std::cout << "Mean Delay: " << i->second.delaySum.GetSeconds()/(i->second.rxPackets) << std::endl;
				std::cout << "Offered Load: " << i->second.txBytes * 8.0 / (i->second.timeLastTxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
				std::cout << "Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstRxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
				std::cout << "Mean jitter:" << i->second.jitterSum.GetSeconds () / (i->second.rxPackets - 1) << std::endl;
				std::cout<<std::endl;

			} 
			else if(t.sourceAddress == "10.1.1.1") {
				throughput_tcp = i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstRxPacket.GetSeconds ()) / 1000000;
				delay_tcp = i->second.delaySum.GetSeconds()/(i->second.rxPackets);
				
				dataset_tcp.Add (tcpPacketSize,throughput_tcp);
				dataset_tcp_delay.Add(tcpPacketSize,delay_tcp);

				std::cout << prot <<" Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
				std::cout << "Tx Packets: " << i->second.txPackets << "\n";
				std::cout << "Tx Bytes:" << i->second.txBytes << "\n";
				std::cout << "Rx Packets: " << i->second.rxPackets << "\n";
				std::cout << "Rx Bytes:" << i->second.rxBytes << "\n";
				std::cout << "Net Packet Lost: " << i->second.lostPackets << "\n";
				std::cout << "Lost due to droppackets: " << i->second.packetsDropped.size() << "\n";
				std::cout << "Max throughput: " << mapMaxThroughput["/NodeList/0/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
				std::cout << "Max throughput: " << mapMaxThroughput["/NodeList/1/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
				std::cout << "Max throughput: " << mapMaxThroughput["/NodeList/2/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
				std::cout << "Max throughput: " << mapMaxThroughput["/NodeList/3/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
				std::cout << "Max throughput: " << mapMaxThroughput["/NodeList/4/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
				std::cout << "Max throughput: " << mapMaxThroughput["/NodeList/5/$ns3::Ipv4L3Protocol/Rx"] << std::endl;
				std::cout << "Delay: " << i->second.delaySum.GetSeconds() << std::endl;
				std::cout << "Mean Delay: " << i->second.delaySum.GetSeconds()/(i->second.rxPackets) << std::endl;
				std::cout << "Offered Load: " << i->second.txBytes * 8.0 / (i->second.timeLastTxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
				std::cout << "Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstRxPacket.GetSeconds ()) / 1000000 << " Mbps" << std::endl;
				std::cout << "Mean jitter:" << i->second.jitterSum.GetSeconds () / (i->second.rxPackets - 1) << std::endl;
				std::cout<<std::endl;
			}
		}
		// std::cout<<"command line Arguments\n";		
	 //    std::cout<<"packetsize: "<<packetsize<<"\n";
	 //    std::cout<<"prot: "<<prot<<"\n";
	 //    std::cout<<"run time: "<<run_time<<"\n";
	 //    std::cout<<"for_loop: "<<for_loop<<"\n";
	 //    std::cout<<"simultaneously: "<<simultaneously<<"\n";
	 //    std::cout<<"Run "<<i<<"finished\n";
		Simulator::Destroy ();
	}

	std::string simultaneously_str = std::to_string(simultaneously);
	std::string fileNameWithNoExtension = prot+simultaneously_str;
	std::string graphicsFileName        = fileNameWithNoExtension + ".png";
	std::string plotFileName            = fileNameWithNoExtension + ".plt";
	std::string plotTitle               = prot + "vs UDP throughput";
	
	std::string fileNameWithNoExtension_delay = prot+"_delay"+simultaneously_str;
	std::string graphicsFileName_delay        = fileNameWithNoExtension_delay + ".png";
	std::string plotFileName_delay            = fileNameWithNoExtension_delay + ".plt";
	std::string plotTitle_delay               = prot + "vs UDP delay";

	// Instantiate the plot and set its title.
	Gnuplot plot (graphicsFileName);
	Gnuplot plot_delay (graphicsFileName_delay);
	
	plot.SetTitle (plotTitle);
	plot_delay.SetTitle (plotTitle_delay);

	// Make the graphics file, which the plot file will create when it
	// is used with Gnuplot, be a PNG file.
	plot.SetTerminal ("png");
	plot_delay.SetTerminal ("png");

	// Set the labels for each axis.
	plot.SetLegend ("Packet Size(in Bytes)", "Throughput Values(in mbps)");
	plot_delay.SetLegend ("Packet Size(in Bytes)", "Delay(in s)");

	// Set the range for the x axis.
	// plot.AppendExtra ("set xrange [-6:+6]");

	// Instantiate the dataset, set its title, and make the points be
	// plotted along with connecting lines.
	dataset_tcp.SetTitle ("Throughput FTP over TCP");
	dataset_tcp.SetStyle (Gnuplot2dDataset::LINES_POINTS);
	dataset_udp.SetTitle ("Throughput CBR over UDP");
	dataset_udp.SetStyle (Gnuplot2dDataset::LINES_POINTS);
	
	dataset_tcp_delay.SetTitle ("Delay FTP over TCP");
	dataset_tcp_delay.SetStyle (Gnuplot2dDataset::LINES_POINTS);
	dataset_udp_delay.SetTitle ("Delay CBR over UDP");
	dataset_udp_delay.SetStyle (Gnuplot2dDataset::LINES_POINTS);

	// double x;
	// double y;


	// Add the dataset to the plot.
	plot.AddDataset (dataset_tcp);
	plot.AddDataset (dataset_udp);
	
	plot_delay.AddDataset (dataset_udp_delay);
	plot_delay.AddDataset (dataset_tcp_delay);

	// Open the plot file.
	std::ofstream plotFile (plotFileName.c_str());

	// Write the plot file.
	plot.GenerateOutput (plotFile);

	// Close the plot file.
	plotFile.close ();

	std::ofstream plotFile_delay (plotFileName_delay.c_str());
	plot_delay.GenerateOutput (plotFile_delay);
	plotFile_delay.close ();

	return 0;
}	