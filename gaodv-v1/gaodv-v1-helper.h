/*
 * GAODV-V1 Helper for NS-3
 * Creates GaodvV1 RoutingProtocol instances with geometric and energy features
 */

#ifndef GAODV_V1_HELPER_H
#define GAODV_V1_HELPER_H

#include "ns3/ipv4-routing-helper.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/object-factory.h"

namespace ns3 {

/**
 * \ingroup gaodvv1
 * \brief Helper class for GAODV-V1 routing protocol with geometric and energy features
 */
class GaodvV1Helper : public Ipv4RoutingHelper
{
  public:
    GaodvV1Helper();

    /**
     * \returns pointer to clone of this GaodvV1Helper
     */
    GaodvV1Helper* Copy() const override;

    /**
     * \param node the node on which the routing protocol will run
     * \returns a newly-created routing protocol
     */
    Ptr<Ipv4RoutingProtocol> Create(Ptr<Node> node) const override;

    /**
     * \param name the name of the attribute to set
     * \param value the value of the attribute to set.
     */
    void Set(std::string name, const AttributeValue& value);

  private:
    ObjectFactory m_agentFactory;
};

} // namespace ns3

#endif /* GAODV_V1_HELPER_H */
