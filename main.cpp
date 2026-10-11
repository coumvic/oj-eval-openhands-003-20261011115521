#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;

enum class Status {
    Accepted,
    Wrong_Answer,
    Runtime_Error,
    Time_Limit_Exceed
};

Status stringToStatus(const string& statusStr) {
    if (statusStr == "Accepted") return Status::Accepted;
    if (statusStr == "Wrong_Answer") return Status::Wrong_Answer;
    if (statusStr == "Runtime_Error") return Status::Runtime_Error;
    if (statusStr == "Time_Limit_Exceed") return Status::Time_Limit_Exceed;
    return Status::Wrong_Answer; // default
}

string statusToString(Status status) {
    switch (status) {
        case Status::Accepted: return "Accepted";
        case Status::Wrong_Answer: return "Wrong_Answer";
        case Status::Runtime_Error: return "Runtime_Error";
        case Status::Time_Limit_Exceed: return "Time_Limit_Exceed";
    }
    return "Wrong_Answer";
}

struct Submission {
    string problemName;
    string teamName;
    Status status;
    int time;
    
    Submission(const string& p, const string& t, Status s, int tm) 
        : problemName(p), teamName(t), status(s), time(tm) {}
};

struct ProblemStatus {
    int wrongAttempts = 0;
    int solveTime = -1;
    int submissionsAfterFreeze = 0;
    bool isFrozen = false;
    bool solvedBeforeFreeze = false;
    
    void addSubmission(Status status, int time, bool frozen) {
        if (solvedBeforeFreeze) return; // already solved before freeze
        
        if (frozen) {
            submissionsAfterFreeze++;
        } else {
            if (status == Status::Accepted && solveTime == -1) {
                solveTime = time;
                solvedBeforeFreeze = true;
            } else if (status != Status::Accepted && solveTime == -1) {
                wrongAttempts++;
            }
        }
    }
    
    void freeze() {
        if (!solvedBeforeFreeze) {
            isFrozen = true;
        }
    }
    
    void unfreeze() {
        isFrozen = false;
    }
    
    int getPenaltyTime() const {
        if (solveTime == -1) return 0;
        return 20 * wrongAttempts + solveTime;
    }
    
    string getDisplayStatus() const {
        if (isFrozen) {
            if (wrongAttempts == 0) {
                return "0/" + to_string(submissionsAfterFreeze);
            }
            return "-" + to_string(wrongAttempts) + "/" + to_string(submissionsAfterFreeze);
        } else if (solvedBeforeFreeze) {
            if (wrongAttempts == 0) return "+";
            return "+" + to_string(wrongAttempts);
        } else {
            if (wrongAttempts == 0) return ".";
            return "-" + to_string(wrongAttempts);
        }
    }
};

struct Team {
    string name;
    map<char, ProblemStatus> problems;
    vector<Submission> submissions;
    
    Team() = default;
    Team(const string& n) : name(n) {}
    
    void addSubmission(const Submission& sub, bool frozen) {
        submissions.push_back(sub);
        problems[sub.problemName[0]].addSubmission(sub.status, sub.time, frozen);
    }
    
    int getSolvedCount() const {
        int count = 0;
        for (const auto& p : problems) {
            if (p.second.solvedBeforeFreeze) count++;
        }
        return count;
    }
    
    int getTotalPenaltyTime() const {
        int total = 0;
        for (const auto& p : problems) {
            total += p.second.getPenaltyTime();
        }
        return total;
    }
    
    vector<int> getSolveTimes() const {
        vector<int> times;
        for (const auto& p : problems) {
            if (p.second.solveTime != -1) {
                times.push_back(p.second.solveTime);
            }
        }
        sort(times.rbegin(), times.rend());
        return times;
    }
    
    bool hasFrozenProblems() const {
        for (const auto& p : problems) {
            if (p.second.isFrozen) return true;
        }
        return false;
    }
    
    char getSmallestFrozenProblem() const {
        char smallest = 'Z' + 1;
        for (const auto& p : problems) {
            if (p.second.isFrozen && p.first < smallest) {
                smallest = p.first;
            }
        }
        return smallest;
    }
};

struct Competition {
    bool started = false;
    bool ended = false;
    int duration = 0;
    int problemCount = 0;
    bool frozen = false;
    bool scoreboardFlushed = false;
    map<string, Team> teams;
    vector<string> teamRanking;
    
    void addTeam(const string& name) {
        if (started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        if (teams.find(name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }
        teams[name] = Team(name);
        cout << "[Info]Add successfully.\n";
    }
    
    void start(int dur, int probCount) {
        if (started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }
        started = true;
        duration = dur;
        problemCount = probCount;
        
        // Initialize problems for all teams
        for (auto& team : teams) {
            for (char c = 'A'; c < 'A' + probCount; c++) {
                team.second.problems[c] = ProblemStatus();
            }
        }
        
        // Initial ranking by lexicographic order
        for (const auto& team : teams) {
            teamRanking.push_back(team.first);
        }
        sort(teamRanking.begin(), teamRanking.end());
        
        cout << "[Info]Competition starts.\n";
    }
    
    void submit(const string& problem, const string& team, Status status, int time) {
        if (teams.find(team) == teams.end()) return;
        teams[team].addSubmission(Submission(problem, team, status, time), frozen);
    }
    
    void flush() {
        if (!started) return;
        calculateRankings();
        scoreboardFlushed = true;
        cout << "[Info]Flush scoreboard.\n";
    }
    
    void freeze() {
        if (!started) return;
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }
        frozen = true;
        
        // Freeze all unsolved problems for all teams
        for (auto& team : teams) {
            for (auto& problem : team.second.problems) {
                if (!problem.second.solvedBeforeFreeze) {
                    problem.second.freeze();
                }
            }
        }
        
        cout << "[Info]Freeze scoreboard.\n";
    }
    
    void calculateRankings() {
        teamRanking.clear();
        for (const auto& team : teams) {
            teamRanking.push_back(team.first);
        }
        
        sort(teamRanking.begin(), teamRanking.end(), [this](const string& a, const string& b) {
            const Team& teamA = teams.at(a);
            const Team& teamB = teams.at(b);
            
            int solvedA = teamA.getSolvedCount();
            int solvedB = teamB.getSolvedCount();
            
            if (solvedA != solvedB) return solvedA > solvedB;
            
            int penaltyA = teamA.getTotalPenaltyTime();
            int penaltyB = teamB.getTotalPenaltyTime();
            
            if (penaltyA != penaltyB) return penaltyA < penaltyB;
            
            vector<int> timesA = teamA.getSolveTimes();
            vector<int> timesB = teamB.getSolveTimes();
            
            int minSize = min(timesA.size(), timesB.size());
            for (int i = 0; i < minSize; i++) {
                if (timesA[i] != timesB[i]) return timesA[i] < timesB[i];
            }
            
            return a < b;
        });
    }
    
    void scroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }
        
        cout << "[Info]Scroll scoreboard.\n";
        
        // Flush first
        flush();
        
        // Print scoreboard before scrolling
        printScoreboard();
        
        vector<string> rankingChanges;
        
        // Continue unfreezing until no frozen problems remain
        while (true) {
            // Find lowest-ranked team with frozen problems
            string targetTeam = "";
            char targetProblem = 'Z' + 1;
            
            for (int i = teamRanking.size() - 1; i >= 0; i--) {
                const string& teamName = teamRanking[i];
                if (teams[teamName].hasFrozenProblems()) {
                    char smallestFrozen = teams[teamName].getSmallestFrozenProblem();
                    if (targetTeam == "" || smallestFrozen < targetProblem) {
                        targetTeam = teamName;
                        targetProblem = smallestFrozen;
                    }
                }
            }
            
            if (targetTeam == "") break; // No more frozen problems
            
            // Record old ranking
            vector<string> oldRanking = teamRanking;
            
            // Unfreeze the problem
            teams[targetTeam].problems[targetProblem].unfreeze();
            
            // Recalculate rankings
            calculateRankings();
            
            // Check if ranking changed
            if (oldRanking != teamRanking) {
                // Find the team that moved up
                for (int i = 0; i < teamRanking.size(); i++) {
                    if (teamRanking[i] == targetTeam) {
                        int newRank = i + 1;
                        int oldRank = 0;
                        for (int j = 0; j < oldRanking.size(); j++) {
                            if (oldRanking[j] == targetTeam) {
                                oldRank = j + 1;
                                break;
                            }
                        }
                        
                        if (newRank < oldRank && newRank <= oldRanking.size()) {
                            string displacedTeam = oldRanking[newRank - 1];
                            int solved = teams[targetTeam].getSolvedCount();
                            int penalty = teams[targetTeam].getTotalPenaltyTime();
                            rankingChanges.push_back(targetTeam + " " + displacedTeam + " " + 
                                                   to_string(solved) + " " + to_string(penalty));
                        }
                        break;
                    }
                }
            }
        }
        
        // Print ranking changes
        for (const string& change : rankingChanges) {
            cout << change << "\n";
        }
        
        // Print final scoreboard
        printScoreboard();
        
        frozen = false;
    }
    
    void printScoreboard() {
        for (int i = 0; i < teamRanking.size(); i++) {
            const string& teamName = teamRanking[i];
            const Team& team = teams[teamName];
            
            cout << teamName << " " << (i + 1) << " " << team.getSolvedCount() 
                 << " " << team.getTotalPenaltyTime();
            
            for (char c = 'A'; c < 'A' + problemCount; c++) {
                cout << " " << team.problems.at(c).getDisplayStatus();
            }
            cout << "\n";
        }
    }
    
    void queryRanking(const string& teamName) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query ranking.\n";
        
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }
        
        for (int i = 0; i < teamRanking.size(); i++) {
            if (teamRanking[i] == teamName) {
                cout << "[" << teamName << "] NOW AT RANKING [" << (i + 1) << "]\n";
                return;
            }
        }
    }
    
    void querySubmission(const string& teamName, const string& problem, const string& status) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query submission.\n";
        
        const Team& team = teams[teamName];
        const vector<Submission>& submissions = team.submissions;
        
        Submission* lastMatch = nullptr;
        
        for (auto it = submissions.rbegin(); it != submissions.rend(); ++it) {
            bool problemMatch = (problem == "ALL" || it->problemName == problem);
            bool statusMatch = (status == "ALL" || statusToString(it->status) == status);
            
            if (problemMatch && statusMatch) {
                lastMatch = const_cast<Submission*>(&(*it));
                break;
            }
        }
        
        if (lastMatch == nullptr) {
            cout << "Cannot find any submission.\n";
        } else {
            cout << "[" << teamName << "] [" << lastMatch->problemName << "] [" 
                 << statusToString(lastMatch->status) << "] [" << lastMatch->time << "]\n";
        }
    }
    
    void end() {
        cout << "[Info]Competition ends.\n";
        ended = true;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    Competition competition;
    string line;
    
    while (getline(cin, line)) {
        if (line.empty()) continue;
        
        istringstream iss(line);
        string command;
        iss >> command;
        
        if (command == "ADDTEAM") {
            string teamName;
            iss >> teamName;
            competition.addTeam(teamName);
        } else if (command == "START") {
            string durationStr, problemStr;
            int duration, problemCount;
            iss >> durationStr >> duration >> problemStr >> problemCount;
            competition.start(duration, problemCount);
        } else if (command == "SUBMIT") {
            string problem, by, team, with, statusStr, at;
            int time;
            iss >> problem >> by >> team >> with >> statusStr >> at >> time;
            competition.submit(problem, team, stringToStatus(statusStr), time);
        } else if (command == "FLUSH") {
            competition.flush();
        } else if (command == "FREEZE") {
            competition.freeze();
        } else if (command == "SCROLL") {
            competition.scroll();
        } else if (command == "QUERY_RANKING") {
            string teamName;
            iss >> teamName;
            competition.queryRanking(teamName);
        } else if (command == "QUERY_SUBMISSION") {
            string teamName, where, problemStr, andStr, statusStr;
            iss >> teamName >> where >> problemStr >> andStr >> statusStr;
            
            // Parse problem and status
            string problem = problemStr.substr(8); // Remove "PROBLEM="
            string status = statusStr.substr(7);   // Remove "STATUS="
            
            competition.querySubmission(teamName, problem, status);
        } else if (command == "END") {
            competition.end();
            break;
        }
    }
    
    return 0;
}