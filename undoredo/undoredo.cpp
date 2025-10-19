#include <iostream>
#include <string>
#include <vector> // Note: You may not use std::vector or other containers for your stacks.
#include <sstream>

std::vector<std::string> get_args(const std::string& command) {
    std::vector<std::string> args;
    std::stringstream ss(command);
    std::string arg;
    bool in_quotes = false;
    std::string current_arg;

    for (char c : command) {
        if (c == ' ' && !in_quotes) {
            if (!current_arg.empty()) {
                args.push_back(current_arg);
                current_arg.clear();
            }
        } else if (c == '"') {
            in_quotes = !in_quotes;
        } else {
            current_arg += c;
        }
    }
    if (!current_arg.empty()) {
        args.push_back(current_arg);
    }
    return args;
}

int main() {
    struct Action {
    std::string prev;   // string *before* the action
    std::string next;   // string *after* the action
    int weight{0};      // weight of the action
};

struct Node {
    Action act;
    Node* prev;
    Node* next;
    explicit Node(const Action& a): act(a), prev(nullptr), next(nullptr) {}
};

// Weighted undo stack that can drop from the bottom (oldest) when overweight.
class WeightedUndoStack {
public:
    WeightedUndoStack(): top_(nullptr), bottom_(nullptr),
                         totalWeight_(0), maxWeight_(0) {}

    void setMaxWeight(int mw) { maxWeight_ = (mw < 0 ? 0 : mw); trim(); }

    void clear() {
        while (top_) {
            Node* t = top_;
            top_ = top_->prev;
            delete t;
        }
        top_ = bottom_ = nullptr;
        totalWeight_ = 0;
    }

    bool empty() const { return top_ == nullptr; }

    void push(const Action& a) {
        Node* n = new Node(a);
        n->prev = top_;
        if (top_) top_->next = n; else bottom_ = n;
        top_ = n;
        totalWeight_ += a.weight;
        trim();
    }

    Action pop() {
        Node* n = top_;
        Action a = n->act;
        top_ = n->prev;
        if (top_) top_->next = nullptr; else bottom_ = nullptr;
        totalWeight_ -= a.weight;
        if (totalWeight_ < 0) totalWeight_ = 0;
        delete n;
        return a;
    }

private:
    Node* top_;
    Node* bottom_;
    int totalWeight_;
    int maxWeight_;

    void trim() {
        while (bottom_ && totalWeight_ > maxWeight_) {
            Node* b = bottom_;
            totalWeight_ -= b->act.weight;
            bottom_ = b->next;
            if (bottom_) bottom_->prev = nullptr; else top_ = nullptr;
            delete b;
        }
        if (totalWeight_ < 0) totalWeight_ = 0;
    }
};

// Simple LIFO stack for redo (no weight bound needed)
class RedoStack {
public:
    RedoStack(): head_(nullptr) {}
    ~RedoStack() { clear(); }

    void clear() {
        while (head_) {
            Node* t = head_;
            head_ = head_->prev;
            delete t;
        }
    }

    bool empty() const { return head_ == nullptr; }

    void push(const Action& a) {
        Node* n = new Node(a);
        n->prev = head_;
        head_ = n;
    }

    Action pop() {
        Node* n = head_;
        Action a = n->act;
        head_ = n->prev;
        delete n;
        return a;
    }

private:
    Node* head_;
};

int main() {
    std::string current;
    bool created = false;
    int maxWeight = 0;

    WeightedUndoStack undoStack;
    RedoStack redoStack;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        std::vector<std::string> args = get_args(line);
        if (args.empty()) continue;

        std::string command = args[0];

        if (command == "CREATE") {
            if (args.size() < 3) continue;
            maxWeight = std::stoi(args[1]);
            current = args[2];                 // already de-quoted by get_args
            undoStack.clear();
            redoStack.clear();
            undoStack.setMaxWeight(maxWeight);
            created = true;
            continue;
        }

        if (!created) continue;  // ignore commands before CREATE

        if (command == "APPEND") {
            if (args.size() < 2) continue;
            std::string add = args[1];

            Action a;
            a.prev = current;
            current += add;
            a.next = current;
            a.weight = static_cast<int>(add.size());

            undoStack.push(a);
            redoStack.clear();
        }
        else if (command == "REPLACE") {
            if (args.size() < 3) continue;
            char findc = args[1].empty() ? '\0' : args[1][0];
            char repc  = args[2].empty() ? '\0' : args[2][0];

            Action a;
            a.prev = current;
            int count = 0;
            for (size_t i = 0; i < current.size(); ++i) {
                if (current[i] == findc) {
                    current[i] = repc;
                    ++count;
                }
            }
            a.next = current;
            a.weight = count;

            undoStack.push(a);
            redoStack.clear();
        }
        else if (command == "DELETE") {
            if (args.size() < 2) continue;
            int idx = std::stoi(args[1]);
            if (idx < 0) idx = 0;
            if (idx > static_cast<int>(current.size())) idx = static_cast<int>(current.size());

            Action a;
            a.prev = current;
            int removed = static_cast<int>(current.size()) - idx;
            current.erase(static_cast<size_t>(idx));
            a.next = current;
            a.weight = removed;

            undoStack.push(a);
            redoStack.clear();
        }
        else if (command == "UNDO") {
            if (undoStack.empty()) {
                std::cout << "Error: Nothing to undo.\n";
            } else {
                Action a = undoStack.pop();
                current = a.prev;
                // move to redo so it can be re-applied
                redoStack.push(a);
            }
        }
        else if (command == "REDO") {
            if (redoStack.empty()) {
                std::cout << "Error: Nothing to redo.\n";
            } else {
                Action a = redoStack.pop();
                current = a.next;
                // push back to undo (enforcing weight limit)
                undoStack.push(a);
            }
        }
        else if (command == "PRINT") {
            std::cout << current << "\n";
        }
    }

    return 0;
}