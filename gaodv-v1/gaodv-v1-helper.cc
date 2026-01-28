/*
 * GAODV-V1 Helper Implementation
 */

#include "gaodv-v1-helper.h"
#include "ns3/gaodv-v1-routing-protocol.h"
#include "ns3/node.h"
#include "ns3/ipv4-routing-protocol.h"

namespace ns3 {

GaodvV1Helper::GaodvV1Helper()
    : Ipv4RoutingHelper()
{
    m_agentFactory.SetTypeId("ns3::gaodvv1::RoutingProtocol");
}

GaodvV1Helper*
GaodvV1Helper::Copy() const
{
    return new GaodvV1Helper(*this);
}

Ptr<Ipv4RoutingProtocol>
GaodvV1Helper::Create(Ptr<Node> node) const
{
    Ptr<gaodvv1::RoutingProtocol> agent = m_agentFactory.Create<gaodvv1::RoutingProtocol>();
    // node->AggregateObject(agent); // Removed to avoid double initialization with ListRouting
    return agent;
}

void
GaodvV1Helper::Set(std::string name, const AttributeValue& value)
{
    m_agentFactory.Set(name, value);
}

} // namespace ns3
