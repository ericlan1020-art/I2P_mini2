#include <utility>
#include "state.hpp"
#include "minimax.hpp"
#include <algorithm>
#include <iostream>


/*============================================================
 * MiniMax — eval_ctx
 *
 * Negamax without pruning. Caller manages memory.
 *============================================================*/
int MiniMax::eval_ctx(
    State *state,
    int depth,
    GameHistory& history,
    int ply,
    SearchContext& ctx,
    const MMParams& p,
    int alpha,
    int beta
){
    if(state->game_state == WIN){
        return P_MAX - ply;
    }
    if(state->game_state == DRAW){
        return 0;
    }
    //if(state->game_state != UNKNOWN){
    //    return 0;
    //}
    ctx.nodes++;
    std::cout << "NODES = " << ctx.nodes << std::endl;
    if(ply > ctx.seldepth){
        ctx.seldepth = ply;
    }
    if(ctx.stop){
        return 0;
    }

    /* === Lazy move generation (sets game_state) === */
    if(state->legal_actions.empty() && state->game_state == UNKNOWN){
        state->get_legal_actions();
    }

    /* === Terminal / leaf checks === */

    // [ Hackathon TODO 3-1 ]
    // return the score for a winning terminal state
    // Hint: prefer faster wins by using ply.
    //---------------------|
    //if(state->game_state == WIN){
    //    return P_MAX - ply;
        //return 0;
        
    //}
    //---------------------^

    //if(state->game_state == DRAW){
    //    return 0;
    //}

    /* === Repetition check (game-specific) === */
    history.push(state->hash());
    int rep_score;
    if(state->check_repetition(history, rep_score)){
        history.pop(state->hash());
        return rep_score;
    }
    

    if(depth <= 0){
        int score = state->evaluate(
            p.use_kp_eval, p.use_eval_mobility, &history
        ); 
        history.pop(state->hash());
        return score;
    }

    /* === Negamax loop === */
    //int best_score = M_MAX;
    int best_score=-P_MAX;

    std::cout << "DEPTH=" << depth
          << " LEGAL=" << state->legal_actions.size()
          << std::endl;
    for(auto& action : state->legal_actions){
        // [ Hackathon TODO 3-2 ]
        // create the child state after applying action
        //----------------|
        State* next = state->next_state(action);
        //----------------^
        std::cout << "depth=" << depth
          << " legal=" << state->legal_actions.size()
          << " child_legal=" << next->legal_actions.size()
          << std::endl;
        //---|
        std::cout << "PARENT: " << state->player
           << " CHILD: " << next->player << std::endl;
        //---^

        bool same = next->same_player_as_parent();

        // [Hackathon TODO 3-3]
        // search the child one level deeper
        //----------------|
        int raw = -eval_ctx(next, depth - 1, history, ply + 1, ctx, p, -beta, -alpha);
        //----------------^

        // [Hackathon TODO 3-4]
        // convert raw to the current player's perspective.
        //----------------|
        int score = raw;
        //----------------^

        delete next;

        // [ Hackathon TODO 3-5 ]
        // update best_score if this child is better.
        //----------------|
        if(score > best_score){
            best_score = score;
        }

        if(best_score > alpha){
            alpha = best_score;
        }

        if(alpha >= beta){
            break;
        }
        //----------------^

    }

    history.pop(state->hash());
    return best_score;
}


/*============================================================
 * MiniMax — search
 *
 * Iterate legal moves, call eval_ctx, return SearchResult.
 *============================================================*/
SearchResult MiniMax::search(
    State *state,
    int depth,
    GameHistory& history,
    SearchContext& ctx
){
    std::cerr << "SEARCH CALLED depth=" << depth << std::endl;
    std::cerr << "ENTER SEARCH depth=" << depth
          << " legal=" << state->legal_actions.size()
          << std::endl;
    std::cerr << ">>> ENTER SEARCH <<<" << std::endl;
    std::cout
        << "player=" << state->player
        << " legal=" << state->legal_actions.size()
        << " game_state=" << state->game_state
        << std::endl;
    std::cout << "[search] depth=" << depth 
          << " legal=" << state->legal_actions.size()
          << " stop=" << ctx.stop
          << std::endl;
    std::cout << "LEGAL MOVES: " << state->legal_actions.size() << std::endl;
    //-----|
    if(state->game_state != UNKNOWN){
        SearchResult result;
        result.score = 0;

        if(state->legal_actions.empty()){
            result.best_move = Move(Point(0,0), Point(0,0));
        } else {
            result.best_move = state->legal_actions[0];
        }

        result.depth = depth;
        return result;
    }
    //-----^
    ctx.reset();
    //---|
    if(depth == 0){
        depth = 7;  // 或你想要的 default depth
    }
    //---^
    MMParams p = MMParams::from_map(ctx.params);
    SearchResult result;
    result.depth = depth;

    if(!state->legal_actions.size()){
        state->get_legal_actions();
    }

    auto& moves = state->legal_actions;
    int self = state->player;
    int opp = 1 - self;

    std::sort(moves.begin(), moves.end(),
        [&](const Move& a, const Move& b) {

            int ar = a.second.first, ac = a.second.second;
            int br = b.second.first, bc = b.second.second;

            bool capA = state->board.board[opp][ar][ac] != 0;
            bool capB = state->board.board[opp][br][bc] != 0;

            return capA > capB;
        }
    );

    //int best_score = M_MAX - 10;
    int best_score = -P_MAX;
    int move_index = 0;
    int total_moves = (int)moves.size();

    for(auto& action : moves){
        /* [ Hackathon TODO 4-1 ]
         * search this move like TODO 3, but starting from the root */
        //------------------|
        State* next = state->next_state(action);

        int score = -eval_ctx(next, depth - 1, history, 1, ctx, p, -P_MAX, P_MAX);

        delete next;
        //------------------^
            if(score > best_score){
                // [ Hackathon TODO 4-2 ]
                // keep this move if it is the best so far
                //----------------------|
                best_score = score;
                result.best_move = action;
                //----------------------^
                if(p.report_partial && ctx.on_root_update){
                   ctx.on_root_update({result.best_move, best_score, depth, move_index + 1, total_moves});
                }
            }  
        move_index++;
    }

    // [ Hackathon TODO 4-3 ]
    // update result and return
    //--------------------|
    if(state->legal_actions.empty()){
        result.score = 0;
        result.best_move = Move(Point(0,0), Point(0,0));
        return result;
    }

    result.score = best_score;

    // 保底（只在沒選到時）
    if(result.best_move.first.first == 0 &&
    result.best_move.first.second == 0 &&
    result.best_move.second.first == 0 &&
    result.best_move.second.second == 0)
    {
        if(!state->legal_actions.empty()){
            result.best_move = state->legal_actions[0];
        }
    }

    return result;
} 


/*============================================================
 * MiniMax — default_params / param_defs
 *============================================================*/
ParamMap MiniMax::default_params(){
    return {
        {"UseKPEval", "true"},
        {"UseEvalMobility", "true"},
        {"ReportPartial", "true"},
    };
}

std::vector<ParamDef> MiniMax::param_defs(){
    return {
        {"UseKPEval", ParamDef::CHECK, "true"},
        {"UseEvalMobility", ParamDef::CHECK, "true"},
        {"ReportPartial", ParamDef::CHECK, "true"},
    };
}
