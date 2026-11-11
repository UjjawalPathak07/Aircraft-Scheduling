#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <sstream>
using namespace std;

const int MIN_SEPARATION = 5; // minutes to move a slot forward

// MODULE 1: AIRCRAFT REQUEST MODULE
enum PriorityType { SCHEDULED = 0, LOW_FUEL = 1, EMERGENCY = 2 };

struct Aircraft {
    int id;
    string callSign;
    int requestedTime;
    int duration;
    PriorityType priority;
    int assignedStart; // -1 = not scheduled yet
    int assignedEnd;
};

Aircraft createAircraftRequest(int id, string callSign, int requestedTime,
                               int duration, PriorityType priority) {
    Aircraft a;
    a.id = id;
    a.callSign = callSign;
    a.requestedTime = requestedTime;
    a.duration = duration;
    a.priority = priority;
    a.assignedStart = -1;
    a.assignedEnd = -1;
    return a;
}

bool validateRequest(const Aircraft &a) {
    if (a.id < 0) return false;
    if (a.callSign.empty()) return false;
    if (a.requestedTime < 0) return false;
    if (a.duration <= 0) return false;
    return true;
}

const char* priorityName(PriorityType priority) {
    switch (priority) {
        case SCHEDULED: return "Scheduled";
        case LOW_FUEL: return "Low fuel";
        case EMERGENCY: return "Emergency";
    }
    return "Unknown";
}

// Times are stored as minutes from midnight; include the day after midnight.
string formatTime(int minutes) {
    ostringstream out;
    int day = minutes / (24 * 60);
    int timeOfDay = minutes % (24 * 60);
    out << setfill('0') << setw(2) << timeOfDay / 60 << ':'
        << setw(2) << timeOfDay % 60;
    if (day > 0) out << " (day +" << day << ')';
    return out.str();
}

void printQueued(const Aircraft &a) {
    cout << "Queued " << a.callSign << " (ID " << a.id
         << "): priority " << priorityName(a.priority)
         << ", requested start " << formatTime(a.requestedTime)
         << ", duration " << a.duration
         << " minutes." << endl;
}

// Overlap test for half-open intervals [s1,e1) and [s2,e2)
bool overlaps(int s1, int e1, int s2, int e2) {
    return s1 < e2 && s2 < e1;
}

// MODULE 2: PRIORITY QUEUE MANAGER (Binary Max-Heap)
class PriorityQueueManager {
private:
    vector<Aircraft> heap;

    int parent(int i) { return (i - 1) / 2; }
    int leftChild(int i) { return 2 * i + 1; }
    int rightChild(int i) { return 2 * i + 2; }

    bool isHigherPriority(const Aircraft &a, const Aircraft &b) {
        if (a.priority != b.priority)
            return a.priority > b.priority;
        return a.requestedTime < b.requestedTime;
    }

    void siftUp(int i) {
        while (i > 0 && isHigherPriority(heap[i], heap[parent(i)])) {
            swap(heap[i], heap[parent(i)]);
            i = parent(i);
        }
    }

    void siftDown(int i) {
        int n = heap.size();
        while (true) {
            int largest = i;
            int l = leftChild(i), r = rightChild(i);
            if (l < n && isHigherPriority(heap[l], heap[largest])) largest = l;
            if (r < n && isHigherPriority(heap[r], heap[largest])) largest = r;
            if (largest == i) break;
            swap(heap[i], heap[largest]);
            i = largest;
        }
    }

public:
    bool isEmpty() { return heap.empty(); }

    void insert(const Aircraft &a) {
        heap.push_back(a);
        siftUp(heap.size() - 1);
        printQueued(a);
    }

    Aircraft extractMax() {
        Aircraft top = heap[0];
        heap[0] = heap.back();
        heap.pop_back();
        if (!heap.empty()) siftDown(0);
        return top;
    }

    Aircraft peek() { return heap[0]; }
};

// MODULE 3: SLOT CONFLICT CHECKER (Plain BST)
struct SlotNode {
    int start, end;
    int aircraftId;
    SlotNode* left;
    SlotNode* right;

    SlotNode(int s, int e, int id) {
        start = s; end = e; aircraftId = id;
        left = right = nullptr;
    }
};

class SlotConflictChecker {
private:
    SlotNode* root;

    SlotNode* insertNode(SlotNode* node, int start, int end, int aircraftId) {
        if (node == nullptr) return new SlotNode(start, end, aircraftId);
        if (start < node->start)
            node->left = insertNode(node->left, start, end, aircraftId);
        else
            node->right = insertNode(node->right, start, end, aircraftId);
        return node;
    }

    SlotNode* findPredecessor(SlotNode* node, int start) {
        SlotNode* result = nullptr;
        while (node != nullptr) {
            if (node->start <= start) {
                result = node;
                node = node->right;
            } else {
                node = node->left;
            }
        }
        return result;
    }

    SlotNode* findSuccessor(SlotNode* node, int start) {
        SlotNode* result = nullptr;
        while (node != nullptr) {
            if (node->start > start) {
                result = node;
                node = node->left;
            } else {
                node = node->right;
            }
        }
        return result;
    }

    void destroy(SlotNode* node) {
        if (node == nullptr) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

public:
    SlotConflictChecker() { root = nullptr; }
    ~SlotConflictChecker() { destroy(root); }
    SlotConflictChecker(const SlotConflictChecker &) = delete;
    SlotConflictChecker &operator=(const SlotConflictChecker &) = delete;

    void insert(int start, int end, int aircraftId) {
        root = insertNode(root, start, end, aircraftId);
    }
    
    void clear() {
        destroy(root);
        root = nullptr;
    }

    bool hasConflict(int start, int duration) {
        int end = start + duration;
        SlotNode* before = findPredecessor(root, start);
        if (before != nullptr && overlaps(start, end, before->start, before->end))
            return true;
        SlotNode* after = findSuccessor(root, start);
        if (after != nullptr && overlaps(start, end, after->start, after->end))
            return true;
        return false;
    }
};

// MODULE 4: TIMELINE MANAGER (Doubly Linked List)
struct ScheduleNode {
    Aircraft ac;
    ScheduleNode* prev;
    ScheduleNode* next;

    ScheduleNode(const Aircraft &a) {
        ac = a;
        prev = next = nullptr;
    }
};

class TimelineManager {
private:
    ScheduleNode* head;
    ScheduleNode* tail;

public:
    TimelineManager() { head = tail = nullptr; }

    ~TimelineManager() {
        while (head != nullptr) {
            ScheduleNode* t = head;
            head = head->next;
            delete t;
        }
    }
    TimelineManager(const TimelineManager &) = delete;
    TimelineManager &operator=(const TimelineManager &) = delete;

    void insertSorted(const Aircraft &a) {
        ScheduleNode* node = new ScheduleNode(a);
        if (head == nullptr) {
            head = tail = node;
            return;
        }
        ScheduleNode* cur = head;
        while (cur != nullptr && cur->ac.assignedStart < a.assignedStart) {
            cur = cur->next;
        }
        if (cur == head) {
            node->next = head;
            head->prev = node;
            head = node;
        } else if (cur == nullptr) {
            tail->next = node;
            node->prev = tail;
            tail = node;
        } else {
            node->prev = cur->prev;
            node->next = cur;
            cur->prev->next = node;
            cur->prev = node;
        }
    }

    void remove(int aircraftId) {
        ScheduleNode* cur = head;
        while (cur != nullptr) {
            if (cur->ac.id == aircraftId) {
                if (cur->prev != nullptr) cur->prev->next = cur->next;
                else head = cur->next;
                if (cur->next != nullptr) cur->next->prev = cur->prev;
                else tail = cur->prev;
                delete cur;
                return;
            }
            cur = cur->next;
        }
    }

    vector<Aircraft> findOverlapping(int winStart, int winEnd) {
        vector<Aircraft> result;
        for (ScheduleNode* cur = head; cur != nullptr; cur = cur->next) {
            if (overlaps(cur->ac.assignedStart, cur->ac.assignedEnd, winStart, winEnd))
                result.push_back(cur->ac);
        }
        return result;
    }

    bool findById(int aircraftId, Aircraft &result) const {
        for (ScheduleNode* cur = head; cur != nullptr; cur = cur->next) {
            if (cur->ac.id == aircraftId) {
                result = cur->ac;
                return true;
            }
        }
        return false;
    }

    void rebuildChecker(SlotConflictChecker &checker) {
        for (ScheduleNode* cur = head; cur != nullptr; cur = cur->next)
            checker.insert(cur->ac.assignedStart, cur->ac.assignedEnd, cur->ac.id);
    }

    void printSchedule() {
        if (head == nullptr) cout << "  No aircraft scheduled." << endl;
        for (ScheduleNode* cur = head; cur != nullptr; cur = cur->next) {
            cout << "  " << cur->ac.callSign << " (ID " << cur->ac.id
                 << "): start " << formatTime(cur->ac.assignedStart) << ", end "
                 << formatTime(cur->ac.assignedEnd) << endl;
        }
    }
};

// MODULE 6: SCHEDULER CONTROLLER (greedy first-fit)
void scheduleAircraft(PriorityQueueManager &pq, TimelineManager &timeline,
                      SlotConflictChecker &checker) {
    while (!pq.isEmpty()) {
        Aircraft a = pq.extractMax();
        int candidateTime = a.requestedTime;
        while (checker.hasConflict(candidateTime, a.duration)) {
            candidateTime += MIN_SEPARATION;
        }
        a.assignedStart = candidateTime;
        a.assignedEnd = candidateTime + a.duration;
        checker.insert(a.assignedStart, a.assignedEnd, a.id);
        timeline.insertSorted(a);
        cout << "Assigned " << a.callSign << " (ID " << a.id
             << "): start " << formatTime(a.assignedStart) << ", end "
             << formatTime(a.assignedEnd) << "." << endl;
    }
}

// MODULE 5: DISRUPTION HANDLER
vector<Aircraft> findAffectedAircraft(TimelineManager &timeline,
                                      int closureStart, int closureEnd) {
    return timeline.findOverlapping(closureStart, closureEnd);
}

Aircraft handleEmergency(Aircraft newAircraft) {
    newAircraft.priority = EMERGENCY;
    return newAircraft;
}

// Removes affected aircraft, rebuilds the BST, blocks the closure and re-fits them.
void rescheduleAffected(vector<Aircraft> &affected, PriorityQueueManager &pq,
                        TimelineManager &timeline, SlotConflictChecker &checker,
                        int closureStart, int closureEnd) {
    for (Aircraft &a : affected) {
        timeline.remove(a.id);
        a.assignedStart = -1;
        a.assignedEnd = -1;
    }
    checker.clear();
    timeline.rebuildChecker(checker);
    checker.insert(closureStart, closureEnd, -1);
    for (Aircraft &a : affected) {
        pq.insert(a);
    }
    scheduleAircraft(pq, timeline, checker);
}

// DEMO through MAIN
int main() {
    PriorityQueueManager pq;
    TimelineManager timeline;
    SlotConflictChecker checker;

    vector<Aircraft> requests = {
        createAircraftRequest(1, "AI101", 100, 20, SCHEDULED),
        createAircraftRequest(2, "AI202", 105, 15, EMERGENCY),
        createAircraftRequest(3, "AI303", 100, 10, SCHEDULED),
        createAircraftRequest(4, "", -5, 0, LOW_FUEL), // invalid on purpose
        createAircraftRequest(5, "AI505", 300, 20, LOW_FUEL)
    };

    cout << "=== Request validation and queue ===" << endl;
    for (Aircraft &a : requests) {
        if (validateRequest(a)) {
            pq.insert(a);
        } else {
            cout << "Rejected request (ID " << a.id << "):" << endl;
            if (a.id < 0) cout << "  ID must be non-negative; received " << a.id << "." << endl;
            if (a.callSign.empty()) cout << "  Call sign must not be empty." << endl;
            if (a.requestedTime < 0) cout << "  Requested start is invalid (before midnight)." << endl;
            if (a.duration <= 0) cout << "  Duration must be positive; received " << a.duration << " minutes." << endl;
        }
    }

    cout << "\n=== Initial assignments ===" << endl;
    scheduleAircraft(pq, timeline, checker);
    cout << "\n=== Initial schedule ===" << endl;
    timeline.printSchedule();

    cout << "\n=== Emergency arrival ===" << endl;
    cout << "Existing assignments are not preempted; the emergency receives the first available slot checked in 5-minute steps." << endl;
    Aircraft emer = handleEmergency(createAircraftRequest(6, "AI606", 110, 10, SCHEDULED));
    pq.insert(emer);
    scheduleAircraft(pq, timeline, checker);
    cout << "\n=== Schedule after emergency arrival ===" << endl;
    timeline.printSchedule();

    int closeStart = 110, closeEnd = 140;
    vector<Aircraft> affected = findAffectedAircraft(timeline, closeStart, closeEnd);
    vector<Aircraft> oldSlots = affected;
    cout << "\n=== Runway closure ===" << endl;
    cout << "Closure: start " << formatTime(closeStart) << ", end " << formatTime(closeEnd)
         << " (end excluded)." << endl;
    if (affected.empty()) cout << "No aircraft affected." << endl;
    for (const Aircraft &a : oldSlots) {
        cout << "Affected " << a.callSign << " (ID " << a.id
              << "): old start " << formatTime(a.assignedStart) << ", old end "
              << formatTime(a.assignedEnd) << "." << endl;
    }

    cout << "\n=== Closure rescheduling ===" << endl;
    rescheduleAffected(affected, pq, timeline, checker, closeStart, closeEnd);
    cout << "\n=== Closure slot changes ===" << endl;
    if (oldSlots.empty()) cout << "No slot changes." << endl;
    for (const Aircraft &old : oldSlots) {
        Aircraft assigned;
        cout << old.callSign << " (ID " << old.id << "): old start "
              << formatTime(old.assignedStart) << ", old end " << formatTime(old.assignedEnd);
        if (timeline.findById(old.id, assigned)) {
            cout << "; new start " << formatTime(assigned.assignedStart)
                 << ", new end " << formatTime(assigned.assignedEnd);
        } else {
            cout << "; new assignment not found";
        }
        cout << "." << endl;
    }
    cout << "\n=== Schedule after closure ===" << endl;
    timeline.printSchedule();
    return 0;
}