#include <handlers/RPCHelpers.h>
#include <server/SubscriptionManager.h>
#include <server/WsBase.h>

void
SubscriptionManager::subLedger(std::shared_ptr<WsBase>& session)
{
    streamSubscribers_[Ledgers].emplace(std::move(session));
}

void
SubscriptionManager::unsubLedger(std::shared_ptr<WsBase>& session)
{
    streamSubscribers_[Ledgers].erase(session);
}

void
SubscriptionManager::pubLedger(
    ripple::LedgerInfo const& lgrInfo,
    ripple::Fees const& fees,
    std::string const& ledgerRange,
    std::uint32_t txnCount)
{
    boost::json::object pubMsg;

    pubMsg["type"] = "ledgerClosed";
    pubMsg["ledger_index"] = lgrInfo.seq;
    pubMsg["ledger_hash"] = to_string(lgrInfo.hash);
    pubMsg["ledger_time"] = lgrInfo.closeTime.time_since_epoch().count();

    pubMsg["fee_ref"] = toBoostJson(fees.units.jsonClipped());
    pubMsg["fee_base"] = toBoostJson(fees.base.jsonClipped());
    pubMsg["reserve_base"] = toBoostJson(fees.accountReserve(0).jsonClipped());
    pubMsg["reserve_inc"] = toBoostJson(fees.increment.jsonClipped());

    pubMsg["validated_ledgers"] = ledgerRange;
    pubMsg["txn_count"] = txnCount;

    for (auto const& session : streamSubscribers_[Ledgers])
        session->send(boost::json::serialize(pubMsg));
}

void
SubscriptionManager::subTransactions(std::shared_ptr<WsBase>& session)
{
    streamSubscribers_[Transactions].emplace(std::move(session));
}

void
SubscriptionManager::unsubTransactions(std::shared_ptr<WsBase>& session)
{
    streamSubscribers_[Transactions].erase(session);
}

void
SubscriptionManager::subAccount(
    ripple::AccountID const& account,
    std::shared_ptr<WsBase>& session)
{
    accountSubscribers_[account].emplace(std::move(session));
}

void
SubscriptionManager::unsubAccount(
    ripple::AccountID const& account,
    std::shared_ptr<WsBase>& session)
{
    accountSubscribers_[account].erase(session);
}

void
SubscriptionManager::pubTransaction(
    Backend::TransactionAndMetadata const& blob,
    std::uint32_t seq)
{
    auto [tx, meta] = deserializeTxPlusMeta(blob, seq);

    boost::json::object pubMsg;
    pubMsg["transaction"] = toJson(*tx);
    pubMsg["meta"] = toJson(*meta);

    for (auto const& session : streamSubscribers_[Transactions])
        session->send(boost::json::serialize(pubMsg));

    auto journal = ripple::debugLog();
    auto accounts = meta->getAffectedAccounts(journal);

    for (ripple::AccountID const& account : accounts)
        for (auto const& session : accountSubscribers_[account])
            session->send(boost::json::serialize(pubMsg));
}

void
SubscriptionManager::forwardProposedTransaction(
    boost::json::object const& response)
{
    for (auto const& session : streamSubscribers_[TransactionsProposed])
        session->send(boost::json::serialize(response));

    auto transaction = response.at("transaction").as_object();
    auto accounts = getAccountsFromTransaction(transaction);

    for (ripple::AccountID const& account : accounts)
        for (auto const& session : accountProposedSubscribers_[account])
            session->send(boost::json::serialize(response));
}

void
SubscriptionManager::subProposedAccount(
    ripple::AccountID const& account,
    std::shared_ptr<WsBase>& session)
{
    accountProposedSubscribers_[account].emplace(std::move(session));
}

void
SubscriptionManager::unsubProposedAccount(
    ripple::AccountID const& account,
    std::shared_ptr<WsBase>& session)
{
    accountProposedSubscribers_[account].erase(session);
}

void
SubscriptionManager::subProposedTransactions(std::shared_ptr<WsBase>& session)
{
    streamSubscribers_[TransactionsProposed].emplace(std::move(session));
}

void
SubscriptionManager::unsubProposedTransactions(std::shared_ptr<WsBase>& session)
{
    streamSubscribers_[TransactionsProposed].erase(session);
}