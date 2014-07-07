// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "checkpoints.h"

#include "main.h"
#include "uint256.h"

#include <stdint.h>

#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include <boost/foreach.hpp>

namespace Checkpoints
{
    typedef std::map<int, uint256> MapCheckpoints;

    // How many times we expect transactions after the last checkpoint to
    // be slower. This number is a compromise, as it can't be accurate for
    // every system. When reindexing from a fast disk with a slow CPU, it
    // can be up to 20, while when downloading from a slow network with a
    // fast multicore CPU, it won't be much higher than 1.
    static const double SIGCHECK_VERIFICATION_FACTOR = 5.0;

    struct CCheckpointData {
        const MapCheckpoints *mapCheckpoints;
        int64_t nTimeLastCheckpoint;
        int64_t nTransactionsLastCheckpoint;
        double fTransactionsPerDay;
    };

    bool fEnabled = true;

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (      0, uint256("0x00000c257b93a36e9a4318a64398d661866341331a984e2b486414fc5bb16ccd"))
        (  41056, uint256("0x000000001f12305bf0443551030d9f18c5d7b1a6b7eb8e899b1b26fc45924ade"))
        (  81847, uint256("0x00000000c164428877cd4d46e2facc881b6b0a803e44a02c1f3b279ae7d58c32"))
        ( 308484, uint256("0x000000016bd2ef95ae4a456c6114cd7736a4219de5b75b2139c840650144e143"))
        ( 380481, uint256("0x00000003064d1fdbe86f35bfce8c54f88a80ef773e820ca86ae820ed6c4defcc"))
        ( 404998, uint256("0x000000004a815d04f437dd83d84866a8a07865f5b47030668a8096df0615361f"))
        ( 411932, uint256("0x000000001f3c7ec7251ebc1670fb3f772b42e25356fa02468c02c89199617cd5"))
        ( 423094, uint256("0x0000000007001e561197a35026b7c9bbaf0b9a1c918a41d9e7d638e44459f116"))
        ( 443157, uint256("0x000000000b103e119485969439ab2203b5578be3fb8b3aab512ebebaca1bce81"))
        ( 458433, uint256("0x000000000318a428560180bb8166321a6b20ae78fc0a9b3c560d30476859b2b5"))
        ( 464836, uint256("0x00000000079e9a16f173bf610f2ceddc5659aa7e9df2366dea01e346c37f9692"))
        ( 467282, uint256("0x0000000004a17401913be0aa29af7ace3335d58a846938d4fee0c749e4828d1d"))
        ( 473033, uint256("0x000000000515c71eb7c3de0574d5f6c632d8de9053c626aba22ae3a9eff67e9c"))
        ( 538178, uint256("0x000000000a13e56dc5d7962d4e3a852ff24055aa15096085d8173faf95172f4d"))
        ( 621138, uint256("0x0000000016a7d31cabbc6257c53d3b58f82f1a897d79066dabcb5ce5b031f8ca"))
        ( 714001, uint256("0x000000001d2b41db149991d5e01aee448042de6ac94e12c5ae6299e4fb129f5a"))
        ( 797370, uint256("0x000000001b24a2f70ce1e50c19d5f3dd77fbd6e0f0a3eb61b95ceaafb8435636"))
        ( 895901, uint256("0x0000000016db7c64fb4bb6475fbb06dca656d32b7864a2d045612660106d411c"))
        ( 972235, uint256("0x000000004e92bead093b946351cd2e7125d23e36042687497561db00a77b6ae8"))
        ;
    static const CCheckpointData data = {
        &mapCheckpoints,
        1402075019, // * UNIX timestamp of last checkpoint block
        1371986,    // * total number of transactions between genesis and last checkpoint
                    //   (the tx=... number in the SetBestChain debug.log lines)
        2880.0      // * estimated number of transactions per day after checkpoint
    };

    static MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        ( 0, uint256("0x00000e5e37c42d6b67d0934399adfb0fa48b59138abb1a8842c88f4ca3d4ec96"))
        ;
    static const CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        1373481000,
        0,
        2880.0
    };

    static MapCheckpoints mapCheckpointsRegtest =
        boost::assign::map_list_of
        ( 0, uint256("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"))
        ;
    static const CCheckpointData dataRegtest = {
        &mapCheckpointsRegtest,
        0,
        0,
        0
    };

    const CCheckpointData &Checkpoints() {
        if (Params().NetworkID() == CChainParams::TESTNET)
            return dataTestnet;
        else if (Params().NetworkID() == CChainParams::MAIN)
            return data;
        else
            return dataRegtest;
    }

    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (!fEnabled)
            return true;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    // Guess how far we are in the verification process at the given block index
    double GuessVerificationProgress(CBlockIndex *pindex, bool fSigchecks) {
        if (pindex==NULL)
            return 0.0;

        int64_t nNow = time(NULL);

        double fSigcheckVerificationFactor = fSigchecks ? SIGCHECK_VERIFICATION_FACTOR : 1.0;
        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is defined as: 1.0 per transaction before the last checkpoint, and
        // fSigcheckVerificationFactor per transaction after.

        const CCheckpointData &data = Checkpoints();

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->nTime)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return fWorkBefore / (fWorkBefore + fWorkAfter);
    }

    int GetTotalBlocksEstimate()
    {
        if (!fEnabled)
            return 0;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (!fEnabled)
            return NULL;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            std::map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }
}
