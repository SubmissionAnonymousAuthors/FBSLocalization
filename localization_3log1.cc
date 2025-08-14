#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/lte-module.h"
#include "ns3/internet-module.h"
#include "ns3/config-store-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/propagation-module.h"
#include "ns3/antenna-module.h"

#define _USE_MATH_DEFINES
#include <cmath>
#ifndef M_PI 
#define M_PI 3.14159265358979323846 
#endif 

using namespace ns3;


AnimationInterface* anim = nullptr;
NodeContainer ueNodes;
NetDeviceContainer enbDevs;
NodeContainer enbNodes;

std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> colors = {
    {0, 255, 0},   // Green
    {0, 0, 255},   // Blue
    {255, 255, 0}, // Yellow
    {255, 0, 255}, // Magenta
    {255, 0, 0}   // Red
};

std::vector<std::tuple<uint8_t,uint8_t,uint8_t>> boxes_color = {
    {0, 0, 128},   // Navy
    {128, 128, 0}, // Olive Green
    {128, 0, 128}, // Purple
    {0, 128, 128}  // Teal
};

double GetRandomAngle()
{
    Ptr<UniformRandomVariable> randomVar = CreateObject<UniformRandomVariable>();
    return randomVar->GetValue(0, 2 * M_PI); // Random angle between 0 and 2π
}

void UpdateNodeMovement(Ptr<Node> &node,std::tuple<double,double> centering,double radius)
{
    Ptr<MobilityModel> mobilitymodel = node->GetObject<MobilityModel>();
    Vector pos = mobilitymodel->GetPosition(); 
    Vector vel = mobilitymodel->GetVelocity();

    double x_center = std::get<0>(centering);
    double y_center = std::get<1>(centering);

    double distance = std::sqrt(std::pow(pos.x - x_center, 2) + std::pow(pos.y - y_center, 2));

    if (distance >= radius)
    {
        double newAngle = GetRandomAngle();
        double speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);

        vel.x = speed * std::cos(newAngle);
        vel.y = speed * std::sin(newAngle);

        // Apply the new velocity to the MobilityModel
        Ptr<ConstantVelocityMobilityModel> cvMobilityModel = mobilitymodel->GetObject<ConstantVelocityMobilityModel>();
        if (cvMobilityModel)
        {
            cvMobilityModel->SetVelocity(vel);
        }
    }
}

void 
UpdateColors(NodeContainer ueNodes)
{   
    for (uint32_t i=0;i<ueNodes.GetN();i++)
    {
        int cellId = ueNodes.Get(i)->GetDevice(0)->GetObject<LteUeNetDevice>()->GetRrc()->GetCellId();
        anim->UpdateNodeColor(ueNodes.Get(i),std::get<0>(colors[cellId-1]), std::get<1>(colors[cellId-1]), std::get<2>(colors[cellId-1]));
    }
};

std::vector<std::tuple<double,double>> boundary(std::vector<std::tuple<double,double>> boundary_points,int abc)
{
    std::tuple<double,double> return_values;
    std::tuple<double,double> top_left_center = {std::get<0>(boundary_points[0])+(std::get<0>(boundary_points[2])-(std::get<0>(boundary_points[0])))/2,std::get<1>(boundary_points[0])+(std::get<1>(boundary_points[2])-(std::get<1>(boundary_points[0])))/2};
    std::tuple<double,double> bottom_right_center = {std::get<0>(boundary_points[2])+(std::get<0>(boundary_points[1])-std::get<0>(boundary_points[2]))/2,std::get<1>(boundary_points[2])+(std::get<1>(boundary_points[1])-std::get<1>(boundary_points[2]))/2};

    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    double x = rand->GetValue(std::get<0>(top_left_center),std::get<0>(bottom_right_center));
    double y = rand->GetValue(std::get<1>(top_left_center),std::get<1>(bottom_right_center));

    return_values = std::make_tuple(x,y);

    if (abc==1) return {return_values,bottom_right_center};
    else if (abc==2) return {return_values,top_left_center};

    return {return_values};
}

std::vector<std::tuple<double,double>> box_dot(std::vector<std::tuple<double,double>> boundary_points)
{
    std::vector<std::tuple<double,double>> box;

    std::tuple<double,double> top_left_center = {std::get<0>(boundary_points[0])+(std::get<0>(boundary_points[2])-std::get<0>(boundary_points[0]))/2,std::get<1>(boundary_points[0])+(std::get<1>(boundary_points[2])-(std::get<1>(boundary_points[0])))/2};
    std::tuple<double,double> top_right_center = {std::get<0>(boundary_points[2])+(std::get<0>(boundary_points[1])-std::get<0>(boundary_points[2]))/2,std::get<1>(boundary_points[0])+(std::get<1>(boundary_points[2])-std::get<1>(boundary_points[0]))/2};
    std::tuple<double,double> bottom_left_center = {std::get<0>(boundary_points[0])+(std::get<0>(boundary_points[2])-std::get<0>(boundary_points[0]))/2,std::get<1>(boundary_points[2])+(std::get<1>(boundary_points[1])-std::get<1>(boundary_points[2]))/2};
    std::tuple<double,double> bottom_right_center = {std::get<0>(boundary_points[2])+(std::get<0>(boundary_points[1])-std::get<0>(boundary_points[2]))/2,std::get<1>(boundary_points[2])+(std::get<1>(boundary_points[1])-std::get<1>(boundary_points[2]))/2};

    box.push_back(top_left_center);
    box.push_back(top_right_center);
    box.push_back(bottom_left_center);
    box.push_back(bottom_right_center);

    return box;
}

void
RecvMeasurementReportCallback(Ptr<OutputStreamWrapper> stream,std::string context,uint64_t imsi,uint16_t cellId,uint16_t rnti,LteRrcSap::MeasurementReport report)
{   
    uint16_t uedist1,uedist2,uedist3,uedist4,uedist5;
    uedist1=uedist2=uedist3=uedist4=uedist5=0;

    uint16_t rsrp1,rsrp2,rsrp3,rsrp4,rsrp5;
    rsrp1=rsrp2=rsrp3=rsrp4=rsrp5=0;

    uint16_t distance1,distance2,distance3,distance4,distance5;
    distance1=distance2=distance3=distance4=distance5=0;

    int neighbourCount=0;
    
    bool haveMeasResultNeighCells = report.measResults.haveMeasResultNeighCells;
    uint16_t serving_rsrp = report.measResults.measResultPCell.rsrpResult;

    uint16_t x,y,z;
    x=y=z=0;

    Vector position_enb1 = enbNodes.Get(0)->GetObject<MobilityModel>()->GetPosition();
    Vector position_enb2 = enbNodes.Get(1)->GetObject<MobilityModel>()->GetPosition();
    Vector position_enb3 = enbNodes.Get(2)->GetObject<MobilityModel>()->GetPosition();
    Vector position_enb4 = enbNodes.Get(3)->GetObject<MobilityModel>()->GetPosition();


    Vector position_fbs = enbNodes.Get(4)->GetObject<MobilityModel>()->GetPosition();
    switch (cellId)
    {
        case 1:
            rsrp1=serving_rsrp;
            x=position_enb1.x;
            y=position_enb1.y;
            z=position_enb1.x;
            break;
        case 2:
            rsrp2=serving_rsrp;
            x=position_enb2.x;
            y=position_enb2.y;
            z=position_enb2.z;
            break;
        case 3:
            rsrp3=serving_rsrp;
            x=position_enb3.x;
            y=position_enb3.y;
            z=position_enb3.z;
            break;
        case 4:
            rsrp4=serving_rsrp;
            x=position_enb4.x;
            y=position_enb4.y;
            z=position_enb4.z;
            break;
        case 5:
            rsrp5=serving_rsrp;
            x=position_fbs.x;
            y=position_fbs.y;
            z=position_fbs.z;
            break;
    }

    distance1=std::sqrt(std::pow((x-position_enb1.x),2)+std::pow((y-position_enb1.y),2)+std::pow((z-position_enb1.x),2));
    distance2=std::sqrt(std::pow((x-position_enb2.x),2)+std::pow((y-position_enb2.y),2)+std::pow((z-position_enb2.z),2));
    distance3=std::sqrt(std::pow((x-position_enb3.x),2)+std::pow((y-position_enb3.y),2)+std::pow((z-position_enb3.z),2));
    distance4=std::sqrt(std::pow((x-position_enb4.z),2)+std::pow((y-position_enb4.y),2)+std::pow((z-position_enb4.z),2));
    distance5=std::sqrt(std::pow((x-position_fbs.x),2)+std::pow((y-position_fbs.y),2)+std::pow((z-position_fbs.z),2));

    Vector position = ueNodes.Get(imsi-1)->GetObject<MobilityModel>()->GetPosition();

    uedist1=std::sqrt(std::pow((position.x-position_enb1.x),2)+std::pow((position.y-position_enb1.y),2)+std::pow((position.z-position_enb1.z),2));
    uedist2=std::sqrt(std::pow((position.x-position_enb2.x),2)+std::pow((position.y-position_enb2.y),2)+std::pow((position.z-position_enb2.z),2));
    uedist3=std::sqrt(std::pow((position.x-position_enb3.x),2)+std::pow((position.y-position_enb3.y),2)+std::pow((position.z-position_enb3.z),2));
    uedist4=std::sqrt(std::pow((position.x-position_enb4.x),2)+std::pow((position.y-position_enb4.y),2)+std::pow((position.z-position_enb4.z),2));
    uedist5=std::sqrt(std::pow((position.x-position_fbs.x),2)+std::pow((position.y-position_fbs.y),2)+std::pow((position.z-position_fbs.z),2));


    if (haveMeasResultNeighCells)
    {   
        for (std::list<LteRrcSap::MeasResultEutra>::const_iterator it= report.measResults.measResultListEutra.begin(); it != report.measResults.measResultListEutra.end(); ++it) 
        {
            neighbourCount++;

            switch (it->physCellId)
            {
                case 1:
                    rsrp1=it->rsrpResult;
                    break;
                case 2:
                    rsrp2=it->rsrpResult;
                    break;
                case 3:
                    rsrp3=it->rsrpResult;
                    break;
                case 4:
                    rsrp4=it->rsrpResult;
                    break;
                case 5:
                    rsrp5=it->rsrpResult;
                    break;
            }
        }

        *stream->GetStream() << Simulator::Now().GetSeconds() <<"\t"<<imsi<<"\t"<<position.x<<"\t"<<position.y<<"\t"<<position.z<<\
        "\t"<<cellId<<"\t"<<neighbourCount<<\
        "\t"<<rsrp1<<"\t"<<position_enb1.x<<"\t"<<position_enb1.y<<"\t"<<position_enb1.z<<"\t"<<distance1<<"\t"<<uedist1<<\
        "\t"<<rsrp2<<"\t"<<position_enb2.x<<"\t"<<position_enb2.y<<"\t"<<position_enb1.z<<"\t"<<distance2<<"\t"<<uedist2<<\
        "\t"<<rsrp3<<"\t"<<position_enb3.x<<"\t"<<position_enb3.y<<"\t"<<position_enb3.z<<"\t"<<distance3<<"\t"<<uedist3<<\
        "\t"<<rsrp4<<"\t"<<position_enb4.x<<"\t"<<position_enb4.y<<"\t"<<position_enb4.z<<"\t"<<distance4<<"\t"<<uedist4<<\
        "\t"<<rsrp5<<"\t"<<position_fbs.x<<"\t"<<position_fbs.y<<"\t"<<position_fbs.z<<"\t"<<distance5<<"\t"<<uedist5<<\
        "\n";
    }
}

int main (int argc, char* argv[])
{       
    double simTime = 5; 
    // uint8_t linked_devices = 16;

    uint16_t total_enb = 5; 
    uint16_t total_ue = 200; 
    double size_x = 500;
    double size_y = 500;

    uint32_t seed = 1;
    uint64_t run = 1;

    // std::string environment = "Urban";
    // std::string city_size = "Medium";
    // double frequency = 0.15e9;


    double lbs = 46, fbs = 48;

    RngSeedManager::SetSeed(seed);
    RngSeedManager::SetRun(run);

    // Create a stringstream
    std::stringstream animStream;
    // animStream << "localization no-building environment="<<environment<<" city-size="<<city_size<<" frequency="<<frequency<< " grid="<<size_x << "X" << size_y <<" n(UE)="<<total_ue<<" n(eNB+FBS)="<<total_enb<<" lbs="<<lbs << " fbs="<<fbs << " seed="<<seed << " run="<<run;

    animStream << "localization no-building propagation=ThreeLogDistancePropagationLossModel" << " grid="<<size_x << "X" << size_y <<" n(UE)="<<total_ue<<" n(eNB+FBS)="<<total_enb<<" lbs="<<lbs << " fbs="<<fbs << " seed="<<seed << " run="<<run;

    std::string baseFileName = animStream.str();

    // Create two different file names: one for .xml and one for .txt
    std::string xmlFileName = baseFileName + ".xml";  // For XML file
    std::string txtFileName = baseFileName + ".txt";  // For TXT file

    std::tuple<double,double> origin = {0.,0.};
    std::tuple<double,double> center = {size_x/2,size_y/2};
    std::tuple<double,double> half_center = {std::get<0>(center)/2,std::get<1>(center)/2}; 

    std::vector<std::tuple<double,double>>upper_left = {
        {0,0},  
        center, 
        {std::get<0>(origin)+std::get<0>(half_center),std::get<1>(origin)+std::get<1>(half_center)} 
    };

    std::vector<std::tuple<double,double>>upper_right = {
        {std::get<0>(center),0},   
        {size_x,std::get<1>(center)},  
        {std::get<0>(center)+std::get<0>(half_center),std::get<1>(origin)+std::get<1>(half_center)} 
    };

    std::vector<std::tuple<double,double>>lower_left = {
        {0,std::get<1>(center)},    
        {std::get<0>(center),size_y},   
        {std::get<0>(origin)+std::get<0>(half_center),std::get<1>(center)+std::get<1>(half_center)} 
    };

    std::vector<std::tuple<double,double>>lower_right = {
        center,   
        {size_x,size_y},   
        {std::get<0>(center)+std::get<0>(half_center),std::get<1>(center)+std::get<1>(half_center)} 
    };

    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);
    lteHelper->SetHandoverAlgorithmType("ns3::A2A4RsrqHandoverAlgorithm");
    Ptr<Node> pgw = epcHelper->GetPgwNode();

    PointToPointHelper p2ph;
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.255.0.0"), 1);

    enbNodes.Create(total_enb);

    double x,y;
    double z;
    double radius,r_dash,theta_deg,theta_rad;

    double a,b,c,d;

    std::vector<std::tuple<double,double>> return_values;

    for (int i=0;i<total_enb;i++)
    {   
        switch (i)
        {
            case 0:
                    return_values = boundary(upper_left,1);
                    a = std::get<0>(return_values[1]);
                    b = std::get<1>(return_values[1]);
                    break;
            case 1:
                    return_values = boundary(upper_right,0);
                    break;
            case 2:
                    return_values = boundary(lower_left,0);
                    break;
            case 3:
                    return_values = boundary(lower_right,2);
                    c = std::get<0>(return_values[1]);
                    d = std::get<1>(return_values[1]);
                    break;
            case 4:
                    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
                    radius = std::sqrt(std::pow(std::get<0>(center) - c, 2) + std::pow(std::get<1>(center) - d, 2));
                    
                    r_dash = rand->GetValue(0, radius);
                    theta_deg = rand->GetValue(0, 360); 

                    // Convert degrees to radians
                    theta_rad = theta_deg * M_PI / 180.0;

                    // Compute x and y coordinates
                    x = r_dash * std::cos(theta_rad) + std::get<0>(center);
                    y = r_dash * std::sin(theta_rad) + std::get<1>(center);
                    z = 2.5;
                    break;
        }

        if (i!=4)
        {
            x = std::get<0>(return_values[0]);
            y = std::get<1>(return_values[0]);
            z = 40.0;
        }

        ns3::Vector initial(x,y,z);
        ns3::MobilityHelper enbMobility;

        if (i!=4) enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        else enbMobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");

        enbMobility.Install(enbNodes.Get(i));
        enbNodes.Get(i)->GetObject<ns3::MobilityModel> ()->SetPosition (initial);

        if (i==4)
        {
            Ptr<ConstantVelocityMobilityModel> mobilityModel = enbNodes.Get(i)->GetObject<ConstantVelocityMobilityModel>();

            double initialAngle = theta_deg * M_PI / 180.0; // Use initial angle from earlier
            double speed = 10.0; // Example speed
            ns3::Vector velocity(speed * std::cos(initialAngle), speed * std::sin(initialAngle), 0.0);
            mobilityModel->SetVelocity(velocity);
        }
    }
    
    // Config::SetDefault("ns3::LteHelper::PathlossModel", StringValue("ns3::OkumuraHataPropagationLossModel"));
    // lteHelper->SetAttribute("PathlossModel",StringValue("ns3::OkumuraHataPropagationLossModel"));
    // lteHelper->SetAttribute("PathlossModel", StringValue("ns3::OkumuraHataPropagationLossModel"));
    // Config::SetDefault("ns3::OkumuraHataPropagationLossModel::Frequency", DoubleValue(frequency));
    // Config::SetDefault("ns3::OkumuraHataPropagationLossModel::Environment", StringValue(environment));
    // Config::SetDefault("ns3::OkumuraHataPropagationLossModel::CitySize", StringValue(city_size));

    lteHelper->SetAttribute("PathlossModel", StringValue("ns3::ThreeLogDistancePropagationLossModel"));             

    Config::SetDefault("ns3::LteEnbMac::NumberOfRaPreambles", UintegerValue(10));
    Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(320));


    ueNodes.Create(total_ue); 

    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    for (uint32_t i = 0; i < ueNodes.GetN(); i++) 
    {
        x = rand->GetValue(0, size_x);
        y = rand->GetValue(0, size_y);
        z = rand->GetValue(1.5, 2.5);
        ns3::Vector initialPosition(x, y, z);
        ns3::MobilityHelper ueMobility;
        ueMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                                "Mode", StringValue ("Time"),
                                "Time", TimeValue (Seconds (20.0)),  
                                "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"), 
                                "Bounds", RectangleValue (Rectangle (0, size_x, 0, size_y))); 
        ueMobility.Install(ueNodes.Get(i));
        ueNodes.Get(i)->GetObject<ns3::MobilityModel> ()->SetPosition (initialPosition);
    }
  
    Ptr<Node> sgw = epcHelper->GetSgwNode();
    MobilityHelper coreMobility;
    coreMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    ns3::Vector forinternet(size_x+100, 400.0, 0.0); // Set position of node 1 // This is the position of the remoteHost
    ns3::Vector forpggw(size_x+100, 300.0, 0.0); // Set position of node 2  // This is the position of the PGW
    ns3::Vector forsgw(size_x+100, 200.0, 0.0); // Set position of node 3   // This is the position of the SGW
    ns3::Vector formme(size_x+100, 100.0, 0.0); // Set position of node 4   // This is the position of the MME
    coreMobility.Install(remoteHostContainer.Get(0));
    coreMobility.Install(pgw);
    coreMobility.Install(sgw);
    coreMobility.Install(NodeList::GetNode(3)); // This is the MME node
    remoteHostContainer.Get(0)->GetObject<ns3::MobilityModel> ()->SetPosition (forinternet);
    pgw->GetObject<ns3::MobilityModel> ()->SetPosition (forpggw);
    sgw->GetObject<ns3::MobilityModel> ()->SetPosition (forsgw);
    NodeList::GetNode(3)->GetObject<ns3::MobilityModel> ()->SetPosition (formme);

    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    NetDeviceContainer ueDevs = lteHelper->InstallUeDevice(ueNodes);

    for (int i=0;i<enbNodes.GetN();i++)
    {   
        Ptr<LteEnbNetDevice> enbDev = enbDevs.Get(i)->GetObject<LteEnbNetDevice>();

        if (i!=4) 
        {
            enbDev->GetPhy()->SetAttribute("TxPower", DoubleValue(lbs));  
            continue;
        }
        else
        {
            enbDev->GetPhy()->SetAttribute("TxPower", DoubleValue(fbs));  
            continue;
        }
    }

    // Config::SetDefault("ns3::LteHelper::AnrEnabled", BooleanValue(true));
    lteHelper->SetAttribute("AnrEnabled",BooleanValue(true));
    lteHelper->AddX2Interface(enbNodes);

    // Set the transmission power of the FBS node to a higher value
    Ptr<LteEnbNetDevice> enbDevice = enbDevs.Get(4)->GetObject<LteEnbNetDevice>();
    // Ptr<LteEnbPhy> enbPhy = enbDevice->GetPhy();
    // enbPhy->SetAttribute("TxPower", DoubleValue(50.0)); // Set the power in dBm
    Ptr<LteEnbRrc> enbRrc = enbDevice->GetRrc();
    enbRrc->SetAttribute("AdmitHandoverRequest", BooleanValue(false));


    // we install the IP stack on the UEs
    internet.Install(ueNodes);
    // assign IP address to UEs
    for (uint32_t u = 0; u < ueNodes.GetN(); ++u)
    {
        Ptr<Node> ue = ueNodes.Get(u);
        Ptr<NetDevice> ueLteDevice = ueDevs.Get(u);
        Ipv4InterfaceContainer ueIpIface;
        ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueLteDevice));
        Ptr<Ipv4StaticRouting> ueStaticRouting;
        ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }
    
    // Downside of this method is that it uses the distance between the UE and eNodeB to attach the UE to the eNodeB
    // lteHelper->AttachToClosestEnb(ueDevs, enbDevs);
    // lteHelper->Attach(ueDevs);

    // Attach a UE to the closest eNB
    // Iterate over all UEs
    int diff_x,diff_y,diff_z;
    diff_x=diff_y=diff_z=0;

    for (int i = 0; i<ueNodes.GetN();i++)
    {
        // Get the UE node
        Ptr<Node> ueNode = ueNodes.Get(i);
        
        // Variables to find the closest eNB
        Ptr<Node> closestEnbNode = nullptr;
        double minDistance = std::numeric_limits<double>::max(); // Start with a large number

        // Iterate over all eNBs to find the closest one
        for (int j = 0; j<enbNodes.GetN();j++)
        {
            if (j==4)
            {
                continue;
            }

            // Get the eNB node
            Ptr<Node> enbNode = enbNodes.Get(j);

            // Calculate the distance between the UE and the eNB
            Ptr<MobilityModel> ueMobility = ueNode->GetObject<MobilityModel>();
            Ptr<MobilityModel> enbMobility = enbNode->GetObject<MobilityModel>();

            if (ueMobility && enbMobility)
            {
                // Get positions
                Vector uePosition = ueMobility->GetPosition();
                Vector enbPosition = enbMobility->GetPosition();

                // Calculate distance
                diff_x = std::pow(uePosition.x-enbPosition.x,2);
                diff_y = std::pow(uePosition.y-enbPosition.y,2);
                diff_z = std::pow(uePosition.z-enbPosition.z,2);

                double distance = std::sqrt(diff_x+diff_y+diff_z);

                // Update the closest eNB if this one is closer
                if (distance < minDistance)
                {
                    minDistance = distance;
                    closestEnbNode = enbNode;
                }
                
            }
        }
        //  Attach the UE to the closet eNB
        if (closestEnbNode)
        {
            lteHelper->Attach(ueDevs.Get(i),closestEnbNode->GetDevice(0));
        }   
    }

    // NodeContainer nodes;
    // nodes.Create(linked_devices);

    // std::vector<std::tuple<double,double>> box_values;

    // MobilityHelper mobility;
    // Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    // mobility.SetPositionAllocator(positionAlloc);
    // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    // PointToPointHelper pointToPoint;
    // pointToPoint.SetDeviceAttribute("DataRate",StringValue("100Mbps"));
    // pointToPoint.SetChannelAttribute("Delay",StringValue("2ms"));

    // NetDeviceContainer devices;

    // uint8_t multiplier;
    // for (uint8_t i=0;i<4;i++)
    // {   
    //     multiplier = 4*i;
    //     switch (i)
    //     {
    //         case 0:
    //             box_values = box_dot(upper_left);
    //             break;
    //         case 1:
    //             box_values = box_dot(upper_right);
    //             break;
    //         case 2:
    //             box_values = box_dot(lower_left);
    //             break;
    //         case 3:
    //             box_values = box_dot(lower_right);
    //             break;
                
    //     }

    //     // Add positions for the four nodes in the current box
    //     for (uint8_t j = 0; j < 4; j++)
    //     {
    //         positionAlloc->Add(Vector(std::get<0>(box_values[j]), std::get<1>(box_values[j]), 0));
    //     }

    //     // Install devices for the current box (ensure indices are valid)
    //     devices.Add(pointToPoint.Install(nodes.Get(multiplier), nodes.Get(multiplier + 1)));
    //     devices.Add(pointToPoint.Install(nodes.Get(multiplier + 1), nodes.Get(multiplier + 3)));
    //     devices.Add(pointToPoint.Install(nodes.Get(multiplier + 3), nodes.Get(multiplier + 2)));
    //     devices.Add(pointToPoint.Install(nodes.Get(multiplier + 2), nodes.Get(multiplier)));
    // }
    // mobility.Install(nodes);
    
    // Create animation interface
    anim = new AnimationInterface(xmlFileName);
    // Since I have no access to the MME node, I had to use for loop and accessing the Core network nodes by NodeList
    for (uint8_t i=0;i<4;i++)
    {
        anim->UpdateNodeColor(NodeList::GetNode(i),0,255,255);  
        anim->UpdateNodeSize(NodeList::GetNode(i),50.0,50.0);    
        anim->UpdateNodeDescription(NodeList::GetNode(i), std::to_string(i+1));
    }
    // Set up node colors and sizes based on eNodeB ID
    for (uint32_t i=0;i<enbNodes.GetN();i++)
    {
        anim->UpdateNodeColor(enbNodes.Get(i),std::get<0>(colors[i]), std::get<1>(colors[i]), std::get<2>(colors[i]));
        anim->UpdateNodeSize(enbNodes.Get(i),50.0,50.0);     
        anim->UpdateNodeDescription(NodeList::GetNode(i+4), std::to_string(i+1));
    }
    //Set up node colors and sizes based on UE ID
    for (uint32_t i=0;i<ueNodes.GetN();i++)
    {
        int cellId = ueNodes.Get(i)->GetDevice(0)->GetObject<LteUeNetDevice>()->GetRrc()->GetCellId();
        anim->UpdateNodeColor(ueNodes.Get(i),std::get<0>(colors[cellId-1]), std::get<1>(colors[cellId-1]), std::get<2>(colors[cellId-1]));
        anim->UpdateNodeSize(ueNodes.Get(i),15.0,15.0);   
        anim->UpdateNodeDescription(NodeList::GetNode(i+9), std::to_string(i+1));
    }

    // Set up box colors
    // multiplier;
    // for (uint32_t i=0;i<4;i++)
    // {   
    //     multiplier = 4*i;
    //     for (uint32_t j=0;j<4;j++)
    //     {  
    //         anim->UpdateNodeColor(nodes.Get(multiplier+j),std::get<0>(boxes_color[j]), std::get<1>(boxes_color[j]), std::get<2>(boxes_color[j]));
    //         anim->UpdateNodeSize(nodes.Get(multiplier+j),10.0,10.0);     
    //         // anim->UpdateNodeDescription(NodeList::GetNode(i+209), std::to_string(i+209));
    //     }
    // }

    lteHelper->EnablePhyTraces();
    lteHelper->EnableMacTraces();
    lteHelper->EnableRlcTraces();
    lteHelper->EnablePdcpTraces();


    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (txtFileName);
    *stream->GetStream() << "Time\tIMSI\tUE-X\tUE-Y\tUE-Z\tServingCellId\tNumOfNeighbour\
    \tRSRP1\tX1\tY1\tZ1\tDistance1\tUE-eNB1-Distance\
    \tRSRP2\tX2\tY2\tZ2\tDistance2\tUE-eNB2-Distance\
    \tRSRP3\tX3\tY3\tZ3\tDistance3\tUE-eNB3-Distance\
    \tRSRP4\tX4\tY4\tZ4\tDistance4\tUE-eNB4-Distance\
    \tRSRP5\tX5\tY5\tZ5\tDistance5\tUE-eNB5-Distance\
    \n";
    
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::LteNetDevice/$ns3::LteEnbNetDevice/LteEnbRrc/RecvMeasurementReport", MakeBoundCallback(&RecvMeasurementReportCallback,stream));


    Simulator::Stop(Seconds(simTime));
    for (double i = 0; i < 5; i+=0.1)
    {   
        Simulator::Schedule(Seconds(i),&UpdateNodeMovement,enbNodes.Get(4),center,radius);
        Simulator::Schedule(Seconds(i), &UpdateColors,ueNodes);  
    }
    Simulator::Run ();
    Simulator::Destroy ();

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    return 0;
}
