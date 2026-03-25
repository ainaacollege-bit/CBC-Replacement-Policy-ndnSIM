// ndn-test1.cpp
// testing 5x5 grid topology

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndnSIM-module.h"

namespace ns3 {

int main(int argc, char* argv[])
{ 
  int runNumber=1;
  CommandLine cmd;
  
  //cmd.AddValue("runNumber", "RNG Run number for the simulation", runNumber);
  std::cout << "Running simulation with RngRun=" << runNumber << std::endl;
  
  cmd.Parse(argc, argv);
  //std::cout << "Running simulation with RngRun=" << runNumber << std::endl;
  // Set up the default random seed and run number (for multiple runs)
  RngSeedManager::SetSeed(12345);  // Global seed for the simulation (fixed across all runs)
  RngSeedManager::SetRun(runNumber);       // Run number (change this for different runs)

  // Create 25 nodes for the grid topology
  NodeContainer nodes;
  nodes.Create(25);

  // Set up the grid structure: 5x5 grid
  PointToPointHelper p2p;
  // p2p.SetChannelAttribute("Delay", StringValue("10ms"));
  p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  
  //Use random variables for link delays:
  Ptr<UniformRandomVariable> randomDelay = CreateObject<UniformRandomVariable>();
  randomDelay->SetAttribute("Min", DoubleValue(5.0));
  randomDelay->SetAttribute("Max", DoubleValue(20.0));
  p2p.SetChannelAttribute("Delay", StringValue(std::to_string(randomDelay->GetValue()) + "ms")); 

  // Connect nodes in a 5x5 grid
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      int index = i * 5 + j;
      if (j < 4) {
        p2p.Install(nodes.Get(index), nodes.Get(index + 1));  // Connect right
      }
      if (i < 4) {
        p2p.Install(nodes.Get(index), nodes.Get(index + 5));  // Connect down
      }
    }
  }
  
  // Create 2 producers (at node 0, 20) and 5 consumers (at node 4, 8, 12, 16, 24)
  Ptr<Node> producer1 = nodes.Get(0);
  Ptr<Node> producer2 = nodes.Get(20);
  
  NodeContainer consumerNodes;
  consumerNodes.Add(nodes.Get(4));
  consumerNodes.Add(nodes.Get(8));
  consumerNodes.Add(nodes.Get(12));
  consumerNodes.Add(nodes.Get(16));
  consumerNodes.Add(nodes.Get(24));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;

  // Set up the content store for each node (cache size)
  ndnHelper.setPolicy("nfd::cs::cbc"); // variables cbc, lru, lrfu, priority_fifo
  ndnHelper.setCsSize(30); // variables 10-50
  ndnHelper.SetDefaultRoutes(false);
  ndnHelper.InstallAll();

  // Install global routing helper on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Install the Producer application on the producer node
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.SetAttribute("Freshness", TimeValue(Seconds(15.0)));
  
  producerHelper.SetPrefix("/data1");
  producerHelper.Install(producer1);
  
  producerHelper.SetPrefix("/data2");
  producerHelper.Install(producer2);
  
  // Set up forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/data1", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::InstallAll("/data2", "/localhost/nfd/strategy/best-route");
  
  // Add /data prefix to global routing table
  ndnGlobalRoutingHelper.AddOrigins("/data1", producer1);
  ndnGlobalRoutingHelper.AddOrigins("/data2", producer2);

  // Install the Consumer application on the consumer node
  
  Ptr<UniformRandomVariable> randomStart = CreateObject<UniformRandomVariable>();
  randomStart->SetAttribute("Min", DoubleValue(0.0));
  randomStart->SetAttribute("Max", DoubleValue(1.0));  // Start time between 0 and 1 second
  
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerZipfMandelbrot");
  consumerHelper.SetAttribute("Frequency", StringValue("100"));  // 100 requests per second
  consumerHelper.SetAttribute("NumberOfContents", StringValue("100"));  // Total number of content items
  
  consumerHelper.SetPrefix("/data1");
  consumerHelper.Install(consumerNodes).Start(Seconds(randomStart->GetValue()));
  
  consumerHelper.SetPrefix("/data2");
  consumerHelper.Install(consumerNodes).Start(Seconds(randomStart->GetValue()));

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();
  
  // Tracer
  //ndn::AppDelayTracer::InstallAll("resultRun/cbcml_regression/consumer-trace/cbcml50-consumer-trace" + std::to_string(runNumber) + ".txt");
  ndn::AppDelayTracer::InstallAll("resultRun/cbcml_regression/cbcml50-consumer-trace.txt");
  ndn::CsTracer::InstallAll("resultRun/cbcml_regression/cbcml50-cache-trace.txt", Seconds(5.0));
  ndn::L3RateTracer::InstallAll("resultRun/cbcml_regression/cbcml50-producer-trace.txt", Seconds(5.0));
  
  // Run the simulation for xx seconds
  Simulator::Stop(Seconds(30.0));
  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}

