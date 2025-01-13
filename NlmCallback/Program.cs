using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices.ComTypes;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using NETWORKLIST;

namespace NlmCallback
{
    internal class Program
    {
        static void Main(string[] args)
        {
            var eventTypes = EventType.INetworkConnectionEvents | EventType.INetworks | EventType.INetworkConnectionCostEvents;
            if (args.Length > 0)
            {
                eventTypes = (EventType)int.Parse(args[0]);
            }
            var events = new NlmEvents();
            events.Subscribe(eventTypes);
            Console.WriteLine("Press ESC to stop");
            while (true)
            {
                if (Console.KeyAvailable)
                {
                    ConsoleKeyInfo key = Console.ReadKey(true);
                    if (key.Key == ConsoleKey.Escape) break;
                }
                Thread.Sleep(100);
            }
            events.Unsubscribe();
        }
    }

    [Flags]
    public enum EventType
    {
        INetworks = 0x1,
        INetworkConnectionEvents = 0x2,
        INetworkConnectionCostEvents = 0x4,
    }

    // It's not flags from COM but actually it is
    [Flags]
    public enum NLM_CONNECTIVITY_FLAGS
    {
        NLM_CONNECTIVITY_DISCONNECTED = 0,
        NLM_CONNECTIVITY_IPV4_NOTRAFFIC = 1,
        NLM_CONNECTIVITY_IPV6_NOTRAFFIC = 2,
        NLM_CONNECTIVITY_IPV4_SUBNET = 0x10,
        NLM_CONNECTIVITY_IPV4_LOCALNETWORK = 0x20,
        NLM_CONNECTIVITY_IPV4_INTERNET = 0x40,
        NLM_CONNECTIVITY_IPV6_SUBNET = 0x100,
        NLM_CONNECTIVITY_IPV6_LOCALNETWORK = 0x200,
        NLM_CONNECTIVITY_IPV6_INTERNET = 0x400
    }

    public class NlmEvents : INetworkEvents, INetworkConnectionEvents, INetworkConnectionCostEvents
    {
        private readonly INetworkListManager _nlm;
        private IConnectionPoint _connectionPoint_INetworkEvents;
        private IConnectionPoint _connectionPoint_INetworkConnectionEvents;
        private IConnectionPoint _connectionPoint_INetworkConnectionCostEvents;
        private int _cookie_INetworkEvents = 0;
        private int _cookie_INetworkConnectionEvents = 0;
        private int _cookie_INetworkConnectionCostEvents = 0;

        public NlmEvents()
        {
            _nlm = new NetworkListManager();
            var cpc = (IConnectionPointContainer)_nlm;
            var riid = typeof(INetworkEvents).GUID;
            cpc.FindConnectionPoint(ref riid, out _connectionPoint_INetworkEvents);
            riid = typeof(INetworkConnectionEvents).GUID;
            cpc.FindConnectionPoint(ref riid, out _connectionPoint_INetworkConnectionEvents);
            riid = typeof(INetworkConnectionCostEvents).GUID;
            cpc.FindConnectionPoint(ref riid, out _connectionPoint_INetworkConnectionCostEvents);
        }

        public void NetworkAdded(Guid networkId)
        {
            var network = _nlm.GetNetwork(networkId);
            Console.WriteLine($"Network: {network.GetName()}:{network.GetDescription()} ({networkId}) added");
        }

        public void NetworkDeleted(Guid networkId)
        {
            var network = _nlm.GetNetwork(networkId);
            Console.WriteLine($"Network: {network.GetName()}:{network.GetDescription()} ({networkId}) deleted");
        }

        public void NetworkConnectivityChanged(Guid networkId, NLM_CONNECTIVITY newConnectivity)
        {
            var network = _nlm.GetNetwork(networkId);
            var newConnectivityFlags = (NLM_CONNECTIVITY_FLAGS)newConnectivity;
            Console.WriteLine($"Network: {network.GetName()} ({networkId}) switches to {newConnectivityFlags}");
        }

        public void NetworkPropertyChanged(Guid networkId, NLM_NETWORK_PROPERTY_CHANGE Flags)
        {
            var network = _nlm.GetNetwork(networkId);
            Console.WriteLine($"Network: {network.GetName()} ({networkId}) {Flags} changed");
        }

        public void Subscribe(EventType t)
        {
            if (t.HasFlag(EventType.INetworks))
                _connectionPoint_INetworkEvents.Advise(this, out _cookie_INetworkEvents);
            if (t.HasFlag(EventType.INetworkConnectionEvents))
                _connectionPoint_INetworkConnectionEvents.Advise(this, out _cookie_INetworkConnectionEvents);
            if (t.HasFlag(EventType.INetworkConnectionCostEvents))
                _connectionPoint_INetworkConnectionCostEvents.Advise(this, out _cookie_INetworkConnectionCostEvents);
        }

        public void Unsubscribe()
        {
            if (_cookie_INetworkEvents != 0)
            {
                _connectionPoint_INetworkEvents.Unadvise(_cookie_INetworkEvents);
                _cookie_INetworkEvents = 0;
            }
            if (_cookie_INetworkConnectionEvents != 0)
            {
                _connectionPoint_INetworkConnectionEvents.Unadvise(_cookie_INetworkConnectionEvents);
                _cookie_INetworkConnectionEvents = 0;
            }
            if (_cookie_INetworkConnectionCostEvents != 0)
            {
                _connectionPoint_INetworkConnectionCostEvents.Unadvise(_cookie_INetworkConnectionCostEvents);
                _cookie_INetworkConnectionCostEvents = 0;
            }
        }

        public void NetworkConnectionConnectivityChanged(Guid connectionId, NLM_CONNECTIVITY newConnectivity)
        {
            var connection = _nlm.GetNetworkConnection(connectionId);
            var newConnectivityFlags = (NLM_CONNECTIVITY_FLAGS)newConnectivity;
            Console.WriteLine($"Connection: {connection.GetNetwork().GetName()} on adapter {connection.GetAdapterId()} switches to {newConnectivityFlags}");
        }

        public void NetworkConnectionPropertyChanged(Guid connectionId, NLM_CONNECTION_PROPERTY_CHANGE Flags)
        {
            var connection = _nlm.GetNetworkConnection(connectionId);
            Console.WriteLine($"Connection: {connection.GetNetwork().GetName()} on adapter {connection.GetAdapterId()} authentication changed");
        }

        public void ConnectionCostChanged(Guid connectionId, uint newCost)
        {
            var connection = _nlm.GetNetworkConnection(connectionId);
            Console.WriteLine($"Connection cost: {connection.GetNetwork().GetName()} on adapter {connection.GetAdapterId()} cost changes to {newCost}");
        }

        public void ConnectionDataPlanStatusChanged(Guid connectionId)
        {
            var connection = _nlm.GetNetworkConnection(connectionId);
            Console.WriteLine($"Connection cost: {connection.GetNetwork().GetName()} on adapter {connection.GetAdapterId()} data plan status changed");
        }
    }
}
