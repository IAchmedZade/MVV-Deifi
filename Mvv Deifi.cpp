#include<unordered_map>
#include<unordered_set>
#include <queue>
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

enum State {
	boarding, readyToMove, waiting, moving, stopped
};

struct DevilishBlockade {
	std::pair<int, int> pos;
	olc::Decal* decal = new olc::Decal(new olc::Sprite("./Sprites/Blockade.png"));

	DevilishBlockade(std::pair<int, int>& pos) : pos(pos) {}
};

struct Passenger {
	int id;
	int timeToStartWorking;
	std::pair<int, int> pos;
	int origin;
	int destination;
	int needLine = 0;
	int needDirection = 0;

	Passenger(int id = -1, int timeToStartWorking = 900, std::pair<int, int> pos = { 0,0 },
		int origin = 0, int dest = 0, int needLine = 0, int needDirection = 1) :
		id(id),
		timeToStartWorking(timeToStartWorking),
		pos(pos),
		origin(origin),
		destination(dest),
		needLine(needLine),
		needDirection(needDirection) {}
};

struct Station {
	olc::Decal* decal = new olc::Decal(new olc::Sprite("./Sprites/Station.png"));
	int id = -1;
	int width = 15;
	int height = 15;
	float angle = 0.f;

	std::pair<int, int> pos;
	std::queue<int> queOnLane1;
	std::queue<int> queOnLane2;
	std::pair<int, int> occupiedLanes{ -1,-1 };


	Station(int id, std::pair<int, int>& pos, float angle = 0.f) : id(id), pos(pos), angle(angle) {}

	void registerTrain(int id, int direction) {
		if (direction == 1) {
			occupiedLanes.first = id;
		}
		else {
			occupiedLanes.second = id;
		}
	}

	void unregisterTrain(int direction) {
		if (direction == 1) {
			occupiedLanes.first = -1;
		}
		else {
			occupiedLanes.second = -1;
		}
	}

	void addIncomingTrain(int trainId, int direction) {
		if (direction == 1) {
			queOnLane1.push(trainId);
		}
		else {
			queOnLane2.push(trainId);
		}
	}

	bool removeIncomingTrain(int trainId, int direction) {
		if (direction == 1) {
			if (queOnLane1.empty() || queOnLane1.front() != trainId) return false;
			queOnLane1.pop();
			return true;
		}
		else {
			if (queOnLane2.empty() || queOnLane2.front() != trainId) return false;
			queOnLane2.pop();
			return true;
		}
	}

	int nextInLine(int direction) {
		if (direction == 1) {
			return queOnLane1.empty() ? -1 : queOnLane1.front();
		}
		else {
			return queOnLane2.empty() ? -1 : queOnLane2.front();
		}
	}

	bool isLaneAvailable(int direction) {
		if (direction == 1) {
			return occupiedLanes.first == -1;
		}
		else {
			return occupiedLanes.second == -1;
		}
	}

	std::pair<int, int> lanePosition(int direction) {
		int x; int y;
		if (direction < 0)
		{
			x = decal->sprite->width / 3;
			y = 3 * decal->sprite->height / 2;
		}
		else
		{
			x = decal->sprite->width / 3;
			y = (-3) * decal->sprite->height / 2;
		}
		return std::pair<int, int>{ pos.first + cos(angle) * x - sin(angle) * y,
			pos.second + sin(angle) * x + cos(angle) * y};
	}
};

struct Train {
	olc::Decal* decal;
	DevilishBlockade* blockade;
	float angle = 0.f;

	int id;
	std::vector<int> line;
	int myLine = 0;
	std::unordered_set<int> myPassengers;
	std::pair<int, int> destination;
	State state = readyToMove;
	int idx = 0;
	int direction = 1;
	std::pair<int, int> pos;

	Train() {}

	Train(int id, std::pair<int, int>& pos, State state, int direction, int myLine) :
		id(id),
		pos(pos),
		state(state),
		direction(direction),
		myLine(myLine) {}

	void updatePositionAndDirection() {
		if ((idx == 0 && direction == -1) || (idx == line.size() - 1 && direction == 1)) {
			direction *= -1;
		}
		else {
			idx += direction;
		}
	}

	void updateDestination() {
		if ((idx == 0 && direction == -1) || (idx == line.size() - 1 && direction == 1)) {
			destination = std::pair<int, int>{ line[idx], -direction };
		}
		else
			destination = std::pair<int, int>{ line[idx + direction], direction };
	}

	bool isBlocked(std::pair<int, int>& movementVector) {
		return dist(addPair(pos, movementVector), blockade->pos) < 20;
	}

	std::pair<int, int> addPair(std::pair<int, int>& pos1, std::pair<int, int>& pos2) {
		return { pos1.first + pos2.first, pos1.second + pos2.second };
	}

	int dist(std::pair<int, int>&& pos1, std::pair<int, int>& pos2) {
		return sqrt(abs(pos1.first - pos2.first) * abs(pos1.first - pos2.first) +
			abs(pos1.second - pos2.second) * abs(pos1.second - pos2.second));
	}
};

struct Graph {
	int commuteTime = 80;
	int globalTime = 0;
	int score = 0;
	std::vector<Train> trains;
	std::vector<DevilishBlockade*> devilishBlockade;
	std::vector<Passenger> slaves;
	std::vector<Station> stations;
	std::vector<std::pair<int, int>> nodes;
	std::vector<std::vector<int>> adjacencyList;
	int stepsize = 10;

	Graph() {}

	Graph(int nTrains, int width, int height) {
		std::pair<int, int> dummy{ width, height };
		for (int id = 0; id < nTrains;++id) {
			trains.push_back(Train(id, dummy, boarding, 1, id));
		}
		int x = width / 2;
		int y = height / 2;
		int spacing = 40;
		// Construct all nodes and edges:
		for (int col = 0; col < 19; ++col) {
			nodes.emplace_back(x + spacing * (col - 12), y);
			stations.emplace_back(col, nodes.back());
		}
		for (int row = 1; row <= 6; ++row) {
			nodes.emplace_back(x - 7 * spacing, y + row * spacing);
			stations.emplace_back(18 + row, nodes.back(), 3.141f / 2.f);
		}
		for (int row = 1; row <= 6; ++row) {
			nodes.emplace_back(x - 4 * spacing, y + spacing * row);
			stations.emplace_back(24 + row, nodes.back(), 3.141f / 2.f);
		}
		for (int row = 4; row > 0; --row) {
			nodes.emplace_back(x + 3 * spacing, y + row * spacing);
			stations.emplace_back(31 + 4 - row, nodes.back(), 3.141f / 2.f);
		}
		for (int row = -1; row >= -5; --row) {
			nodes.emplace_back(x + 3 * spacing, y + spacing * row);
			stations.emplace_back(35 - 1 - row, nodes.back(), 3.141f / 2.f);
		}
		for (int lambda = 1; lambda <= 3; ++lambda) {
			nodes.emplace_back(x - (7 + lambda) * spacing, y + (1 + lambda) * spacing);
			stations.emplace_back(39 + lambda, nodes.back(), 3.f * 3.141f / 4.f);
		}
		for (int lambda = 1; lambda <= 5; ++lambda) {
			nodes.emplace_back(x - (7 + lambda) * spacing, y - lambda * spacing);
			stations.emplace_back(42 + lambda, nodes.back(), 5.f * 3.141f / 4.f);
		}
		for (int lambda = 1; lambda <= 3; ++lambda) {
			nodes.emplace_back(x + (3 + lambda) * spacing, y + (1 + lambda) * spacing);
			stations.emplace_back(47 + lambda, nodes.back(), 3.141f / 4.f);
		}
		for (int lambda = 1; lambda <= 3; ++lambda) {
			nodes.emplace_back(x + (3 + lambda) * spacing, y - lambda * spacing);
			stations.emplace_back(50 + lambda, nodes.back(), 7.f * 3.141f / 4.f);
		}
		// Add Lines to trains
		trains[0].line = std::vector<int>{ 42,41,40,19,5,6,7,8,9,10,11,12,13,14,15,35,36,37,38,39 };
		trains[1].line = std::vector<int>{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18 };
		trains[2].line = std::vector<int>{ 47,46,45,44,43,5,6,7,8,9,10,11,12,13,14,15,51,52,53 };
		trains[3].line = std::vector<int>{ 24,23,22,21,20,19,5,6,7,8,9,10,11,12,13,14,15,34,33,32,31 };
		trains[4].line = std::vector<int>{ 30,29,28,27,26,25,8,9,10,11,12,13,14,15,34,48,49,50 };
		for (auto& train : trains) {
			train.pos = stations[train.line[0]].lanePosition(1);
		}
	}

	void handleTrains() {
		++globalTime;
		if (globalTime > 1e9) globalTime = 0;
		for (auto& train : trains) {
			if (train.state == readyToMove) {
				train.updateDestination();
				train.state = moving;
			}
			else if (train.state == moving) {
				findClosestBlockade(train);
				moveTrain(train);
			}
			else if (train.state == boarding) {
				train.updateDestination();
				stations[train.destination.first].addIncomingTrain(train.id, train.destination.second);
				boardPassengers(train);
				train.state = waiting;
			}
			else if (train.state == waiting) {
				int nextId = stations[train.destination.first].nextInLine(train.direction);
				if ((train.id == nextId || nextId == -1) &&
					stations[train.destination.first].isLaneAvailable(train.destination.second)) {
					train.state = readyToMove;
				}
			}
			else if (train.state == stopped) {
				std::pair<int, int> target = stations[train.destination.first].lanePosition(train.destination.second);
				std::pair<int, int> origin = stations[train.line[train.idx]].lanePosition(train.direction);
				std::pair<int, int> movementVector = { (target.first - origin.first) / stepsize,(target.second - origin.second) / stepsize };
				if (!train.isBlocked(movementVector))
					train.state = moving;
			}
		}
	}

	void findClosestBlockade(Train& train) {
		train.blockade = nullptr;
		for (auto& blockade : devilishBlockade) {
			if (dist(train.pos, blockade->pos) < 10) {
				train.blockade = blockade;
				break;
			}
		}
		train.state = (train.blockade == nullptr ? moving : stopped);
	}

	void moveTrain(Train& train) {
		std::pair<int, int> target = stations[train.destination.first].lanePosition(train.destination.second);
		std::pair<int, int> origin = stations[train.line[train.idx]].lanePosition(train.direction);

		addPair(train.pos, { (target.first - origin.first) / stepsize, (target.second - origin.second) / stepsize });

		if (dist(train.pos, target) < 10) {
			train.pos = target;
			train.angle = stations[train.destination.first].angle;

			if (train.idx == 0 || train.idx == train.line.size() - 1) {
				stations[train.line[train.idx]].occupiedLanes = std::pair<int, int>{ -1,-1 };
			}
			else {
				stations[train.line[train.idx]].unregisterTrain(train.direction);
			}
			stations[train.destination.first].removeIncomingTrain(train.id, train.destination.second);
			stations[train.destination.first].registerTrain(train.id, train.direction);
			train.updatePositionAndDirection();
			train.state = boarding;
		}
	}

	void boardPassengers(Train& train) {
		int currentStationId = stations[train.line[train.idx]].id;
		for (auto& slave : slaves) {
			if (!train.myPassengers.count(slave.id) &&
				slave.origin == currentStationId &&
				slave.needDirection == train.direction &&
				slave.needLine == train.myLine) {
				train.myPassengers.insert(slave.id);
			}
			if (slave.destination == currentStationId && train.myPassengers.count(slave.id)) {
				train.myPassengers.erase(slave.id);
				if (slave.timeToStartWorking < globalTime) ++score;
				for (int i = 0; i < slaves.size(); ++i) {
					if (slaves[i].id == slave.id) {
						slaves.erase(slaves.begin() + i);
						break;
					}
				}
			}
		}
	}

	void generateSlaves() {
		if (slaves.size() < 80) {
			int currentSize = slaves.size();
			for (int i = 0; i < 20; ++i) {
				int l = rand() % 3;
				int start = rand() % trains[l].line.size();
				int end = start;
				while (end == start) {
					end = rand() % trains[l].line.size();
				}
				int originId = trains[l].line[start];
				int destinationId = trains[l].line[end];
				int direction = (start < end ? 1 : -1);
				slaves.emplace_back(currentSize + i, globalTime + 100,
					stations[originId].pos, originId, destinationId,
					l, direction);
			}
		}
	}

	void addPair(std::pair<int, int>& pos1, const std::pair<int, int>& pos2) {
		pos1.first += pos2.first;
		pos1.second += pos2.second;
	}

	void substractPair(std::pair<int, int>& pos1, const std::pair<int, int>& pos2) {
		pos1.first -= pos2.first;
		pos1.second -= pos2.second;
	}

	std::string convertGameTime() {
		int time = globalTime / 10;

		int hours = (6 + time / 60) % 24;
		int minutes = time - 60 * (time / 60);
		std::string mins = minutes < 10 ? "0" + std::to_string(minutes) : std::to_string(minutes);
		return std::to_string(hours) + ":" + mins;
	}

	int dist(std::pair<int, int>& pos1, std::pair<int, int>& pos2) {
		return abs(pos1.first - pos2.first) + abs(pos1.second - pos2.second);
	}
};

struct Bomb {
	std::pair<int, int> pos;
	int timeToDetonation;
	int detonationTime = 5;
	int reach = 20;
	Bomb(std::pair<int, int> pos, int time) : pos(pos), timeToDetonation(time) {}
};

struct Deifi {
public:
	std::pair<int, int> pos;
	int nBombs;

	olc::Sprite* sprite;
	olc::Decal* decal;
};

struct Engel {
	olc::Sprite* sprite;
	olc::Decal* decal;
	int id;
	int stepsize = 30;
	std::pair<int, int> pos;
	std::pair<int, int> oldPos;
	std::vector<std::pair<int, int>> queueOfDetonations;
	std::vector<std::pair<int, int>> path;

	bool Move() {
		if (queueOfDetonations.size()) {
			if (!path.size() && pos != queueOfDetonations[0]) {
				generatePath();
			}
			else if (path.size()) {
				pos = path.back();
				path.pop_back();

				if (!path.size()) {
					return true;
				}
				if (pos.first < 10 || pos.first >  900 ||
					pos.second < 10 || pos.second > 900) {
					oldPos = pos;
					return true;
				}
			}
			else {
				return true;
			}
		}
		return false;
	}

	void generatePath() {
		int approxDist = dist(pos, queueOfDetonations[0]);
		if (approxDist < 100) {
			stepsize = 10;
		}
		else {
			stepsize = 30;
		}
		int delX = (queueOfDetonations[0].first - pos.first) / stepsize;
		int delY = (queueOfDetonations[0].second - pos.second) / stepsize;

		for (int i = 1; i <= stepsize; ++i) {
			path.emplace_back(pos.first + i * delX, pos.second + i * delY);
		}
		path.emplace_back(queueOfDetonations[0]);
		std::reverse(path.begin(), path.end());
	}

	void eraseFirstDetonation() {
		if (queueOfDetonations.size())
			queueOfDetonations.erase(queueOfDetonations.begin());
	}

	double dist(std::pair<int, int>& pos1, std::pair<int, int>& pos2) {
		return sqrt(abs(pos1.first - pos2.first) * abs(pos1.first - pos2.first) + abs(pos1.second - pos2.second) * abs(pos1.second - pos2.second));
	}
};

class App : public olc::PixelGameEngine
{
	Deifi myDeifi;
	Engel mvvRep;
	std::vector<Bomb> tickingBombs;
	std::vector<std::pair<int, int>> detonations;
	olc::Sprite* spr;


	int placeholder = 0;

	olc::Sprite* deifiSprite;
	olc::Decal* deifiDecal;

	Graph graph;

	bool pauseGame = false;
public:
	App()
	{
		sAppName = "Trains";
	}

	bool OnUserCreate() override
	{
		graph = Graph(5, ScreenWidth(), ScreenHeight());
		Clear(olc::BLANK);
		DrawInstructions();
		InitializeDeifiAndMvvRep();

		DrawString(10, 10, "Score: ", olc::RED, 2);
		DrawString(10, 40, "Time: ", olc::RED, 2);
		DrawString(10, 70, "Blocks: ", olc::RED, 2);

		DrawGraphOfStations();
		DrawAllTrains();
		DrawObject(myDeifi, olc::vf2d{ 2.f,1.5f });
		DrawObject(mvvRep);

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		//DrawGraphOfStations();

		DisplayData();

		if (!HandleUserInput())
			return false;

		MoveMvvRep();

		for (auto& blockade : graph.devilishBlockade) {
			DrawBlockade(blockade);
		}

		DrawObject(myDeifi, olc::vf2d{ 2.f,1.5f }, olc::Pixel(min(255, 80 + 2 * graph.score), 100, 150));
		DrawObject(mvvRep);
		graph.handleTrains();
		DrawAllTrains();

		for (auto& station : graph.stations) DrawStation(station);

		return true;
	}

	void DrawInstructions() {
		DrawString(ScreenWidth() / 2 - 50, 20, "Press arrow keys to Move");
		DrawString(ScreenWidth() / 2 - 50, 40, "Press space to setup blockade");
		DrawString(ScreenWidth() / 2 - 50, 60, "Press escape to exit");
	}

	void InitializeDeifiAndMvvRep() {
		myDeifi.pos = std::pair<int, int>{ ScreenWidth() / 2, ScreenHeight() / 2 };
		myDeifi.nBombs = 100;
		myDeifi.sprite = new olc::Sprite("./Sprites/Deifi.png");
		myDeifi.decal = new olc::Decal(myDeifi.sprite);

		mvvRep.id = 1;
		mvvRep.pos.first = ScreenWidth() / 2;
		mvvRep.pos.second = ScreenHeight() / 4;
		mvvRep.oldPos = mvvRep.pos;
		mvvRep.sprite = new olc::Sprite("./Sprites/Engel.png");
		mvvRep.decal = new olc::Decal(mvvRep.sprite);

	}

	void DisplayData() {
		FillRect(110, 10, 200, 30, olc::BLANK);
		DrawString(110, 10, std::to_string(graph.score), olc::RED, 2);
		if (!(graph.globalTime % 10)) {
			FillRect(110, 40, 200, 80, olc::BLANK);
			DrawString(110, 40, graph.convertGameTime(), olc::RED, 2);

			if (!(graph.globalTime % 1000)) {
				graph.generateSlaves();
				for (auto& station : graph.stations) {
					DrawStation(station);
				}
			}
		}
		FillRect(110, 70, 200, 30, olc::BLANK);
		DrawString(110, 70, std::to_string(myDeifi.nBombs), olc::RED, 2);
	}

	bool HandleUserInput()
	{
		if (GetKey(olc::Key::O).bHeld) {
			DrawRotatedDecal(olc::vi2d{ myDeifi.pos.first, myDeifi.pos.second }, myDeifi.decal, graph.globalTime % 360,
				{ 10.f,10.f }, { 2.f,2.f });
		}
		if (GetKey(olc::Key::ESCAPE).bPressed) {
			return false;
		}
		if (GetKey(olc::Key::ENTER).bPressed) {
			int a = 1;
		}
		if (GetKey(olc::Key::LEFT).bHeld) {
			MoveDeifi({ -5,0 });
		}
		if (GetKey(olc::Key::RIGHT).bHeld) {
			MoveDeifi({ 5,0 });
		}
		if (GetKey(olc::Key::UP).bHeld) {
			MoveDeifi({ 0,-5 });
		}
		if (GetKey(olc::Key::DOWN).bHeld) {
			MoveDeifi({ 0,5 });
		}
		if (GetKey(olc::Key::SPACE).bPressed && myDeifi.nBombs && dist(myDeifi.pos, mvvRep.pos) > 20) {
			--myDeifi.nBombs;
			graph.devilishBlockade.push_back(new DevilishBlockade(myDeifi.pos));
			mvvRep.queueOfDetonations.push_back(myDeifi.pos);
		}
		if (GetKey(olc::Key::DEL).bPressed) {
			for (auto& train : graph.trains) {
				if (train.blockade) {
					delete train.blockade->decal;
					train.blockade->decal = new olc::Decal(new olc::Sprite("./Sprites/BlackBox.png"));
					train.blockade->pos = std::pair<int, int>{ 0,0 };
				}
			}
		}
		return true;
	}

	void MoveMvvRep() {
		if (!mvvRep.queueOfDetonations.empty()) {
			if (mvvRep.Move()) {
				for (auto& train : graph.trains) {
					if (train.blockade && train.blockade->pos == mvvRep.queueOfDetonations[0]) {
						//Figure out how to delete sprites and decals in PixelGameEngine
						delete train.blockade->decal;
						train.blockade->decal = new olc::Decal(new olc::Sprite("./Sprites/BlackBox.png"));
						train.blockade->pos = std::pair<int, int>{ ScreenWidth() ,ScreenHeight() };
					}
				}
				for (auto it = graph.devilishBlockade.begin(); it != graph.devilishBlockade.end(); ++it) {
					if ((*it)->pos == mvvRep.queueOfDetonations[0]) {
						graph.devilishBlockade.erase(it);
						break;
					}
				}
				mvvRep.eraseFirstDetonation();
			}
		}
	}

	void DrawGraphOfStations() {
		/*
		int x = ScreenWidth() / 2;
		int y = ScreenHeight() / 2;
		std::pair<int, int> position{ x,y };
		graph.adjacencyList.resize(20);
		int midIdx = 12;
		int rightSplit = 9;
		for (int i = 0; i < midIdx; ++i) {
			AddStation(position, i);
			if (i > 0) {
				graph.adjacencyList[i].push_back(i - 1);
			}
			if (i < 8) {
				graph.adjacencyList[i].push_back(i + 1);
			}
			position.first += 60;
			if (i > 1 && i <= rightSplit) {
				graph.trains[0].line.push_back(i);
				graph.trains[1].line.push_back(i);
			}
			graph.trains[2].line.push_back(i);
		}

		{
			position.first = x + 60;
			position.second = y - 60;

			AddStation(position, midIdx);
			graph.trains[0].line.insert(graph.trains[0].line.begin(), midIdx);
			graph.adjacencyList[midIdx].push_back(2);
			graph.adjacencyList[2].push_back(midIdx);
			++midIdx;

			position.first -= 60;
			position.second = y - 60;

			AddStation(position, midIdx);
			graph.trains[0].line.insert(graph.trains[0].line.begin(), midIdx);
			graph.adjacencyList[midIdx - 1].push_back(midIdx);
			graph.adjacencyList[midIdx].push_back(midIdx - 1);
			++midIdx;

			position.first += 60 * 10;

			AddStation(position, midIdx);
			graph.trains[0].line.push_back(midIdx);
			graph.adjacencyList[midIdx].push_back(rightSplit);
			graph.adjacencyList[rightSplit].push_back(midIdx);
			++midIdx;

			position.first += 60;

			AddStation(position, midIdx);
			graph.trains[0].line.push_back(midIdx);
			graph.adjacencyList[midIdx].push_back(midIdx - 1);
			graph.adjacencyList[midIdx - 1].push_back(midIdx);
			++midIdx;
		}

		{
			position.first = x + 60;
			position.second = y + 60;

			AddStation(position, midIdx);

			graph.trains[1].line.insert(graph.trains[1].line.begin(), midIdx);
			graph.adjacencyList[midIdx].push_back(2);
			graph.adjacencyList[2].push_back(midIdx);
			++midIdx;

			position.first -= 60;

			AddStation(position, midIdx);

			graph.trains[1].line.insert(graph.trains[1].line.begin(), midIdx);
			graph.adjacencyList[midIdx].push_back(midIdx - 1);
			graph.adjacencyList[midIdx - 1].push_back(midIdx);
			++midIdx;

			position.first += 60 * 10;

			AddStation(position, midIdx);
			graph.trains[1].line.push_back(midIdx);
			graph.adjacencyList[midIdx].push_back(rightSplit);
			graph.adjacencyList[rightSplit].push_back(midIdx);
			++midIdx;

			position.first += 60;

			AddStation(position, midIdx);
			graph.trains[1].line.push_back(midIdx);
			graph.adjacencyList[midIdx].push_back(midIdx - 1);
			graph.adjacencyList[midIdx - 1].push_back(midIdx);
			++midIdx;
		}

		//DrawConnections();
		{
			graph.trains[0].id = 0;
			graph.trains[0].myLine = 0;
			graph.trains[0].direction = 1;
			graph.trains[0].pos = graph.stations[graph.trains[0].line[0]].lanePosition(1);
			graph.trains[0].state = boarding;

			graph.trains[1].id = 1;
			graph.trains[1].myLine = 1;
			graph.trains[1].direction = 1;
			graph.trains[1].pos = graph.stations[graph.trains[1].line[0]].lanePosition(1);
			graph.trains[1].state = boarding;

			graph.trains[2].id = 2;
			graph.trains[2].myLine = 2;
			graph.trains[2].direction = 1;
			graph.trains[2].pos = graph.stations[graph.trains[2].line[0]].lanePosition(1);
			graph.trains[2].state = boarding;
		}
		*/
		for (auto& train : graph.trains) {
			train.decal = new olc::Decal(new olc::Sprite("./Sprites/SBahn.png"));
		}
	}

	void AddStation(std::pair<int, int>& pos, int id) {
		graph.stations.emplace_back(id, pos);
		//DrawStation(graph.stations.back());
	}

	template<typename T>
	void DrawObject(T& obj, const olc::vf2d& scale = { 1.f,1.f }, const olc::Pixel& tint = olc::WHITE) {
		DrawDecal(olc::vi2d{ obj.pos.first, obj.pos.second }, obj.decal, scale, tint);
	}

	void DrawStation(Station& station) {
		DrawRotatedDecal(olc::vf2d{ (float)station.pos.first, (float)station.pos.second },
			station.decal, station.angle);
		/*
		if (station.angle) {
			//FillRect(station.pos.first - 5, station.pos.second - 5, 30, 10, olc::RED);
			DrawRotatedDecal(olc::vf2d{ (float)station.pos.first, (float)station.pos.second },
				station.decal, station.angle);
		}
		else if (!station.angle) {
			FillRect(station.pos.first - 5, station.pos.second - 5, 30, 10, olc::RED);
		}*/
		int cnt = 0;
		for (auto& slave : graph.slaves) {
			if (slave.pos == station.pos) {
				FillCircle(station.pos.first + cnt * 5, station.pos.second, 3, olc::GREEN);
				++cnt;
			}
		}
	}

	void DrawConnections() {
		for (int i = 0; i < graph.adjacencyList.size(); ++i) {
			for (int idx : graph.adjacencyList[i]) {
				DrawLine(graph.stations[i].pos.first, graph.stations[i].pos.second,
					graph.stations[idx].pos.first, graph.stations[idx].pos.second, olc::RED);
			}
		}
	}

	void DrawTrain(Train& train) {
		if (train.angle) {
			spr = new olc::Sprite("./Sprites/SBahn.png");
			DrawRotatedDecal(olc::vi2d{ train.pos.first, train.pos.second }, train.decal, train.angle,
				olc::vf2d{ (float)(train.decal->sprite->width) / 2.f, (float)(train.decal->sprite->height) / 2.f }
			);
			//olc::vf2d{ (float)(spr->width / 2),(float)(spr->height / 2) });
		}
		else {
			if (train.myPassengers.size()) {
				DrawDecal(olc::vi2d{ train.pos.first, train.pos.second }, train.decal, { 1.2f,1.2f },
					olc::Pixel(200, 200, 200));
			}
			else {
				DrawDecal(olc::vi2d{ train.pos.first, train.pos.second }, train.decal, { 1.2f,1.2f },
					olc::Pixel(100, 100, 150));
			}
		}
	}

	void DrawAllTrains() {
		for (auto& train : graph.trains) {
			DrawTrain(train);
			if (train.state == waiting) {
				DrawStation(graph.stations[train.line[train.idx]]);
			}
		}
	}

	void MoveDeifi(std::pair<int, int> difference) {
		addPairs(myDeifi.pos, difference);
		DrawObject(myDeifi);
	}

	void DrawBlockade(DevilishBlockade* blockade) {
		DrawDecal(olc::vi2d{ blockade->pos.first, blockade->pos.second }, blockade->decal, { 2.f, 2.f });
	}

	void DrawSlaves() {
		for (auto& slave : graph.slaves) {
			FillCircle(slave.pos.first, slave.pos.second, 3, olc::DARK_GREEN);
		}
	}

	void addPairs(std::pair<int, int>& current, std::pair<int, int>& toAdd) {
		current.first += toAdd.first;
		current.second += toAdd.second;
	}

	int dist(std::pair<int, int>& pos1, std::pair<int, int> pos2) {
		return (int)sqrt((pos1.first - pos2.first) * (pos1.first - pos2.first) +
			(pos1.second - pos2.second) * (pos1.second - pos2.second));
	}
};

int main()
{
	App game;
	if (game.Construct(1024, 730, 4, 4))
		game.Start();
	return 0;
}