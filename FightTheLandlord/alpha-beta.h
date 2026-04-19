#pragma once
#include "Gamer.h"
#include"ParticleFilter.h"
class Gamer;
struct SearchNode {
    Gamer self;
    Gamer player1;
    Gamer player2;
    int turn=0;
    int pass_count = 0;
    SearchNode();
   
    int who() {
        turn = (turn + 1) % 3;
        return turn;
    }

    SearchNode clone()  {
        SearchNode res;
        res.self = self.clone();
        res.player1 = player1.clone();
        res.player2 = player2.clone();
        res.turn = turn;
        res.pass_count = pass_count;
        return res;
    }
    
    bool isGameOver() const {
        return self.totalCards() == 0 ||
            player1.totalCards() == 0 ||
            player2.totalCards() == 0;
    }
};

std::vector<SearchNode> getFirstNode(DouDizhuAI& ai) {
    std::vector<SearchNode> res;
    auto p1 = ai.PF->getParticles(0);
    auto p2 = ai.PF->getParticles(1);
    for (int i = 0; i < p1.size(); i++) {
        SearchNode node;
        node.self = ai.my_hand.clone();
        Gamer player1;
        player1.type = ai.player1_type;
        player1.addCards(p1[i].cards);
        if (player1.type == GamerType::Lord) {
            for (int j = 0; j < 3; j++) {
                if(ai.recorder->isPlayed(ai.public_cards[j])) player1.addCard(ai.public_cards[j]);
            }
        }
        player1.analyze();
        node.player1 = player1;
        Gamer player2;
        player2.type = ai.player2_type;
        player2.addCards(p2[i].cards);
        if (player2.type == GamerType::Lord) {
            for (int j = 0; j < 3; j++) {
                if (ai.recorder->isPlayed(ai.public_cards[j])) player2.addCard(ai.public_cards[j]);
            }
        }
        player2.analyze();
        node.player1 = player1;
        res.push_back(node);
    }
    return res;
}


// 判断当前玩家是否和我同一阵营
bool isMyTeam(SearchNode& node) {
    auto turn=node.who();
    if (turn == 0) return true;
    if(node.self.type==GamerType::Lord) return false;
    if (node.self.type != GamerType::Lord) {
        if(turn==1) return node.player1.type!=GamerType::Lord;
        if(turn == 2) return node.player2.type != GamerType::Lord;
    }
    return false;
}

int alphaBeta(SearchNode& node, int depth, int alpha, int beta) {
    if (depth == 0 || node.isGameOver()) {
        return evaluate(node);  // 不需要传 my_turn，evaluate 可以从 node 中获取
    }

    auto moves = getCandidateMoves(node);

    if (isMyTeam(node)) {
        // ========== 我方回合（MAX层） ==========
        int max_eval = -10000000;
        for (const auto& move : moves) {
            SearchNode child = node.clone();
            child.applyMove(move);
            int eval = alphaBeta(child, depth - 1, alpha, beta);
            max_eval = std::max(max_eval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break;
        }
        return max_eval;
    }
    else {
        // ========== 敌方回合（MIN层） ==========
        int min_eval = 10000000;
        for (const auto& move : moves) {
            SearchNode child = node.clone();
            child.applyMove(move);
            int eval = alphaBeta(child, depth - 1, alpha, beta);
            min_eval = std::min(min_eval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) break;
        }
        return min_eval;
    }
}