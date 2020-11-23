#include "3DPCH.h"
#include "Player.h"
#include"Pickup.h"
#include"SpeedPickup.h"
#include"HeightPickup.h"
#include"CannonPickup.h"
#include"ParticleComponent.h"
#include "Traps.h"

Pickup* getCorrectPickupByID(int id);

Player::Player()
{
	m_movementVector = XMVectorZero();
	m_jumps = 0;
	m_currentDistance = 0;
	m_hasDashed = false;
	m_angleY = 0;
	m_playerEntity = nullptr;
	m_animMesh = nullptr;
	m_cameraTransform = nullptr;
	m_controller = nullptr;
	m_state = PlayerState::IDLE;


	Physics::get().Attach(this, true, false);
	m_currentSpeedModifier = 1.f;
	m_speedModifierTime = 0;
	if (!Pickup::hasInitPickupArray())
	{
		std::vector<Pickup*> vec;
		vec.emplace_back(new SpeedPickup());
		vec.emplace_back(new HeightPickup());
		vec.emplace_back(new CannonPickup());
		Pickup::initPickupArray(vec);
	}

	for (int i = 0; i < 10; i++)
	{
		m_lineData[i].direction = { 0, 0, 0 };
		m_lineData[i].position = { 0, 0, 0 };
	}
	
	//GUI
	m_score = 0;
	GUITextStyle style;
	style.position.y = 70.f;
	style.scale = { 0.5f };
	m_scoreLabelGUIIndex = GUIHandler::get().addGUIText("Score: ", L"squirk.spritefont", style);
	style.position.x = 160.f;
	style.color = Colors::Yellow;
	m_scoreGUIIndex = GUIHandler::get().addGUIText(std::to_string(m_score), L"squirk.spritefont", style);

	GUIImageStyle imageStyle;
	imageStyle.position = Vector2(400.f, 50.f);
	imageStyle.scale = Vector2(0.9f, 0.9f);
	m_instructionGuiIndex = GUIHandler::get().addGUIImage(L"keyboard.png", imageStyle);

	//Test Button stuff
	GUIButtonStyle btnStyle;
	btnStyle.position = Vector2(240, 200);
	btnStyle.scale = Vector2(0.5, 0.5);
	closeInstructionsBtnIndex = GUIHandler::get().addGUIButton(L"closeButton.png", btnStyle);

	//Attach to the click listener for the button
	dynamic_cast<GUIButton*>(GUIHandler::get().getElementMap()->at(closeInstructionsBtnIndex))->Attach(this);

	GUIImageStyle guiInfo;
	guiInfo.origin = Vector2(256, 256);
	guiInfo.position = Vector2(Engine::get().getSettings().width / 2, Engine::get().getSettings().height / 2);
	m_cannonCrosshairID = GUIHandler::get().addGUIImage(L"crosshair.png", guiInfo);
	GUIHandler::get().setVisible(m_cannonCrosshairID, false);




}

Player::~Player()
{
	if (m_playerEntity)
		delete m_playerEntity;

	//Deleted by scene
	//if (m_3dMarker)
	//	delete m_3dMarker;

	Pickup::clearStaticPickupArrayPlz();
}

void Player::setCannonEntity(Entity* entity)
{
	m_cannonEntity = entity;

	if (!m_3dMarker)
	{
		m_3dMarker = new Entity("3DMarker");
		Engine::get().getEntityMap()->emplace("3DMarker", m_3dMarker);
		MeshComponent* mesh = new MeshComponent("testCube_pCube1.lrm");
		mesh->setCastsShadow(false);
		m_3dMarker->addComponent("6 nov (mesh)", mesh);
		Engine::get().getMeshComponentMap()->emplace(1632, mesh);
	}
}

Entity* Player::get3DMarkerEntity()
{
	return m_3dMarker;
}

bool Player::isRunning()
{
	Vector3 vec = m_movementVector;
	return (vec.Length() > 0);
}

void Player::setStates(std::vector<State> states)
{
	m_movementVector = XMVECTOR();
	if (m_state != PlayerState::DASH && m_state != PlayerState::ROLL)
	{
		for (std::vector<int>::size_type i = 0; i < states.size(); i++)
		{
			switch (states[i])
			{
			case WALK_LEFT:
				m_movementVector += m_cameraTransform->getLeftVector();
				break;
			case WALK_RIGHT:
				m_movementVector += m_cameraTransform->getRightVector();
				break;
			case WALK_FORWARD:
				m_movementVector += m_cameraTransform->getForwardVector();
				break;
			case WALK_BACKWARD:
				m_movementVector += m_cameraTransform->getBackwardVector();
				break;
			default:
				break;
			}
		}
	}
}

void Player::handleRotation(const float &dt)
{
	Quaternion currentRotation;
	Quaternion slerped;
	Quaternion targetRot;

	m_movementVector = XMVector3Normalize(m_movementVector);

	if (Vector3(m_movementVector).LengthSquared() > 0) //Only update when moving
		m_angleY = XMVectorGetY(XMVector3AngleBetweenNormals(XMVectorSet(0, 0, 1, 0), m_movementVector));

	if (Vector3(m_movementVector).LengthSquared() > 0)
	{
		//This is the current rotation in quaternions
		currentRotation = m_playerEntity->getRotation();
		currentRotation.Normalize();

		auto cameraRot = m_cameraTransform->getRotation();
		auto offset = Vector4(XMVector3AngleBetweenNormals(XMVector3Normalize(m_movementVector), m_cameraTransform->getForwardVector()));

		//if this vector has posisitv value the character is facing the positiv x axis, checks movementVec against cameraForward
		if (XMVectorGetY(XMVector3Cross(m_movementVector, m_cameraTransform->getForwardVector())) > 0.0f)
		{
			//m_angleY = -m_angleY;
			offset.y = -offset.y;
		}

		//This is the angleY target quaternion
		targetRot = Quaternion(0, cameraRot.y, 0, cameraRot.w) * Quaternion::CreateFromYawPitchRoll(offset.y, 0, 0);//Quaternion::CreateFromYawPitchRoll(m_angleY, 0, 0);
		targetRot.Normalize();

		//Land somewhere in between target and current
		slerped = Quaternion::Slerp(currentRotation, targetRot, dt / 0.08f);
		slerped.Normalize();

		//Display slerped result
		m_playerEntity->setRotationQuat(slerped);

	}
	//m_playerEntity->
}

float lerp(const float& a, const float &b, const float &t)
{
	return a + (t * (b - a));
}
bool doOnce = false;

Vector3 Player::trajectoryEquation(Vector3 pos, Vector3 vel, float t)
{
	Vector3 curPos = pos;
	Vector3 curVel = vel;

	curVel.y -= GRAVITY * m_gravityScale * t;
	if (curVel.y <= -MAX_FALL_SPEED)
		curVel.y = -MAX_FALL_SPEED;
	curPos = curVel * t + pos;

	return curPos;
}

void Player::trajectoryEquationOutFill(Vector3 pos, Vector3 vel, float t, XMFLOAT3& outPos, XMFLOAT3& outDir)
{
	Vector3 curPos = pos;
	Vector3 curVel = vel;

	curVel.y -= GRAVITY * m_gravityScale * t;
	if (curVel.y <= -MAX_FALL_SPEED)
		curVel.y = -MAX_FALL_SPEED;
	curPos = curVel * t + pos;

	outPos = curPos;
	outDir = curVel;
}

Vector3 Player::calculatePath(Vector3 position, Vector3 velocity, float gravityY)
{
	Vector3 returnPosition;

	bool foundEnd = false;
	bool print = !doOnce;
	float t = 0.01;
	Vector3 pos = position;
	Vector3 vel = velocity;

	while (!foundEnd && t < 200)
	{
		Vector3 curPos;
		curPos = trajectoryEquation(pos, vel, t);
		if (print)
		{
			std::cout << (int)curPos.x << " " << (int)curPos.y << " " << (int)curPos.z << std::endl;
			doOnce = true;
		}

		t += 0.5f;
		bool hit = Physics::get().hitSomething(curPos, m_controller->getOriginalRadius(), m_controller->getOriginalHalfHeight());
		if (hit)
		{
			foundEnd = true;
			returnPosition = curPos;
			m_3dMarker->setPosition(returnPosition);
			float tDist = t / 10;
			for (int i = 0; i < 10; i++)
			{
				Vector3 lineDataPos;
				Vector3 lineDataDir;
				float tempT = tDist * i;
				trajectoryEquationOutFill(pos, vel, tempT, m_lineData[i].position, this->m_lineData[i].direction);
			}

		}
		else
			m_3dMarker->setPosition(m_playerEntity->getTranslation());
	}

	ImGui::Text("Found Position:(%d, %d, %d)", (int)pos.x, (int)pos.y, (int)pos.z);

	return returnPosition;
}

void Player::playerStateLogic(const float& dt)
{
	m_velocity = Vector3(XMVector3Normalize(Vector3(XMVectorGetX(m_movementVector), 0, XMVectorGetZ(m_movementVector))) * PLAYER_SPEED * dt * this->m_currentSpeedModifier) + Vector3(0, m_velocity.y, 0);
	switch (m_state)
	{
	case PlayerState::ROLL:
		//std::cout << "ROLL\n";
		if (m_currentDistance >= ROLL_TRAVEL_DISTANCE)
		{
			m_lastState = PlayerState::ROLL;
			m_state = PlayerState::IDLE;
			m_controller->setControllerSize(CAPSULE_HEIGHT);
			m_controller->setControllerRadius(CAPSULE_RADIUS);
			//m_animMesh->playAnimation("Running4.1", true);
			idleAnimation();
		}
		else
		{
			m_currentDistance += ROLL_SPEED * dt;
			Vector3 move = m_moveDirection * ROLL_SPEED * dt;
			move.y += -GRAVITY * dt;
			m_velocity += move;
			//m_controller->move(move, dt);
		}
		break;
	case PlayerState::DASH:
		//std::cout << "DASH\n";
		if (m_currentDistance >= DASH_TRAVEL_DISTANCE)
		{
			m_lastState = PlayerState::DASH;
			m_state = PlayerState::FALLING;
			m_hasDashed = true;
			idleAnimation();
		}
		else
		{
			m_currentDistance += DASH_TRAVEL_DISTANCE * DASH_SPEED * dt;
			m_velocity += m_moveDirection * DASH_SPEED * DASH_TRAVEL_DISTANCE * dt;
			//m_controller->move(m_moveDirection * DASH_SPEED * DASH_TRAVEL_DISTANCE * dt, dt);
		}
		break;
	case PlayerState::FALLING:
		//std::cout << "FALLING\n";
		if (m_jumps == 0) // Can only jump once in air
			m_jumps = ALLOWED_NR_OF_JUMPS - 1;

		if (m_controller->checkGround(m_controller->getFootPosition(), Vector3(0.f, -1.f, 0.f), 0.1f))
		{
			m_lastState = PlayerState::FALLING;
			m_state = PlayerState::IDLE;
			m_jumps = 0;
			m_hasDashed = false;
		}
		//else
		//{
		//	//finalMovement.y += finalMovement.y - 1.f*dt;//-JUMP_SPEED * FALL_MULTIPLIER * dt;
		//	//m_controller->move(finalMovement, dt);
		//}
		break;
	case PlayerState::JUMPING:
		//std::cout << "JUMPING\n";
		//m_velocity.y = JUMP_SPEED * dt * m_playerScale;// * dt;

		//m_currentDistance += JUMP_SPEED * dt;

		if (m_velocity.y < 0.f)
		{
			m_currentDistance = 0.f;
			m_lastState = PlayerState::JUMPING;
			m_state = PlayerState::FALLING;
		}

		//if (m_currentDistance > JUMP_DISTANCE)
		//{
		//	m_currentDistance = 0.f;
		//	m_state = PlayerState::FALLING;
		//}

		break;
	case PlayerState::IDLE:
		//std::cout << "IDLE\n";
		if (!m_controller->checkGround(m_controller->getFootPosition(), Vector3(0.f, -1.f, 0.f), 0.1f))
		{
			m_lastState = PlayerState::IDLE;
			m_state = PlayerState::FALLING;
		}
		break;
	case PlayerState::CANNON:
		m_controller->setPosition(m_cannonEntity->getTranslation());
		m_velocity.y = 0;

		GUIHandler::get().setVisible(m_cannonCrosshairID, false);


		if (m_shouldFire)
		{
			m_state = PlayerState::FLYINGBALL;
			m_lastState = PlayerState::CANNON;
			m_direction = m_cameraTransform->getForwardVector();
			m_velocity = m_direction;
			m_cameraOffset = Vector3(0.f, 0.f, 0.f);
			m_3dMarker->setPosition(0, -9999, -9999);
			m_shouldFire = false;
		}
		else //Draw marker
		{
			Vector3 finalPos;
			finalPos = calculatePath(m_controller->getCenterPosition(), m_cameraTransform->getForwardVector(), GRAVITY);
			
			return;
		}
		break;
	case PlayerState::FLYINGBALL:
		GUIHandler::get().setVisible(m_cannonCrosshairID, false);


		m_velocity.x = m_direction.x;
		m_velocity.z = m_direction.z;
		if (m_controller->checkGround(m_controller->getCenterPosition(), DirectX::XMVector3Normalize(m_velocity), 1.f))
		{
			PlayerMessageData data;
			data.playerActionType = PlayerActions::ON_FIRE_CANNON;
			data.playerPtr = this;

			for (int i = 0; i < m_playerObservers.size(); i++)
			{
				m_playerObservers.at(i)->reactOnPlayer(data);
			}

			m_state = PlayerState::JUMPING;
			m_lastState = PlayerState::FLYINGBALL;
			m_pickupPointer->onDepleted();
			m_pickupPointer->onRemove();
			SAFE_DELETE(m_pickupPointer);
		}
		break;
	default:
		break;
	}

	if (m_state == PlayerState::FALLING || m_state == PlayerState::JUMPING || m_state == PlayerState::ROLL || m_state == PlayerState::FLYINGBALL)
	{
		if (m_playerEntity->getTranslation().y != m_lastPosition.y)
			m_velocity += Vector3(0, -GRAVITY * m_gravityScale, 0);
		else if (m_velocity.y == 0)
			m_velocity += Vector3(0, -GRAVITY * m_gravityScale, 0);
	}

	//std::cout << m_velocity.z << "\n";

	// Max Gravity Tests
	if (m_velocity.y <= -MAX_FALL_SPEED)
		m_velocity.y = -MAX_FALL_SPEED;

	m_controller->move(m_velocity, dt);
	m_lastPosition = m_playerEntity->getTranslation();


	//float vectorLen = Vector3(m_finalMovement.x, 0, m_finalMovement.z).LengthSquared();
	float vectorLen = Vector3(m_velocity.x, 0, m_velocity.z).Length();
	if (m_state != PlayerState::ROLL && m_state != PlayerState::DASH)
	{
		float blend = vectorLen / (PLAYER_SPEED * dt);

		if ((PLAYER_SPEED * dt) <= 0.0f)
			blend = 0.0f;

		m_animMesh->setCurrentBlend( std::fmin(blend, 1.55f) );
		//// analog animation:
		//if (vectorLen > 0)
		//	m_animMesh->setCurrentBlend(1.f);
		//else
		//	m_animMesh->setCurrentBlend(0);
	}

	if (m_controller->getFootPosition().y < (float)m_heightLimitBeforeRespawn)
	{
		respawnPlayer();
	}
}

bool Player::pickupUpdate(Pickup* pickupPtr, const float& dt)
{
	bool shouldRemovePickup = false;

	if (pickupPtr)
	{
		if (pickupPtr->isActive())
		{
			switch (pickupPtr->getPickupType())
			{
			case PickupType::SPEED:
				m_speedModifierTime += dt;
				if (m_speedModifierTime <= FOR_FULL_EFFECT_TIME)
				{
					m_currentSpeedModifier += m_goalSpeedModifier * dt / FOR_FULL_EFFECT_TIME;
				}
				pickupPtr->update(dt);
				if (pickupPtr->isDepleted())
				{
					if (m_goalSpeedModifier > 0.f)
					{
						m_goalSpeedModifier *= -1;;
						m_speedModifierTime = 0.f;
					}

					if (m_speedModifierTime <= FOR_FULL_EFFECT_TIME)
					{
						m_currentSpeedModifier += m_goalSpeedModifier * dt / FOR_FULL_EFFECT_TIME;
						m_currentSpeedModifier = max(1.0f, m_currentSpeedModifier);
					}

					if (pickupPtr->shouldDestroy())
					{
						shouldRemovePickup = true;
						m_currentSpeedModifier = 1.f;
					}
				}
				break;
			case PickupType::HEIGHTBOOST:
				if (m_state == PlayerState::FALLING )
				{
					pickupPtr->onDepleted();
					shouldRemovePickup = true;
				}
				break;
			case PickupType::CANNON:
				//Cannon Update
			default:
				break;
			}
		}
	}
	return shouldRemovePickup;
}

void Player::updatePlayer(const float& dt)
{
	//TRAP UPDATE
	switch (m_activeTrap)
	{
	case TrapType::EMPTY:
		break;
	case TrapType::SLOW:
		m_slowTimer += dt;
		if (m_slowTimer >= m_slowTime)
		{
			m_slowTimer = 0;
			m_currentSpeedModifier = 1;
			m_activeTrap = TrapType::EMPTY;
		}
		break;
	default:
		break;
	}

	//PickupUpdate
	if (pickupUpdate(m_pickupPointer, dt))
	{
		m_pickupPointer->onRemove();
		delete m_pickupPointer;
		m_pickupPointer = nullptr;
	}
	if(pickupUpdate(m_environmentPickup, dt))
	{
		m_environmentPickup->onRemove();
		delete m_environmentPickup;
		m_environmentPickup = nullptr;
	}

	//RotationUpdate
	if(m_state != PlayerState::ROLL)
		handleRotation(dt);

	//PlayerState logic Update
	playerStateLogic(dt);

	ImGui::Begin("Player Information", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
	ImGui::Text("Player Position: (%d %d, %d)", (int)this->getPlayerEntity()->getTranslation().x, (int)this->getPlayerEntity()->getTranslation().y, (int)this->getPlayerEntity()->getTranslation().z);
	ImGui::Text("PlayerState: %d", this->m_state);
	ImGui::End();
}

void Player::setPlayerEntity(Entity* entity)
{
	m_playerEntity = entity;
	m_controller = static_cast<CharacterControllerComponent*>(m_playerEntity->getComponent("CCC"));
	entity->addComponent("ScoreAudio", m_audioComponent = new AudioComponent(m_scoreSound));

	//To test heightpickup faster, if you see this you can remove it
	m_pickupPointer = new CannonPickup();
	m_pickupPointer->onPickup(m_playerEntity);



}

Vector3 Player::getCheckpointPos()
{
	return m_checkpointPos;
}

Vector3 Player::getVelocity()
{
	return m_velocity;
}

LineData* Player::getLineDataArray()
{
	return m_lineData;
}

void Player::setCheckpoint(Vector3 newPosition)
{
	m_checkpointPos = newPosition;
}

void Player::setCameraTranformPtr(Transform* transform)
{
	m_cameraTransform = transform;
}

void Player::setAnimMeshPtr(AnimatedMeshComponent* animatedMesh)
{
	m_animMesh = animatedMesh;
}

void Player::incrementScore()
{
	m_score++;
	GUIHandler::get().changeGUIText(m_scoreGUIIndex, std::to_string(m_score));
}

void Player::increaseScoreBy(int value)
{
	m_score += value;
	GUIHandler::get().changeGUIText(m_scoreGUIIndex, std::to_string(m_score));
}

void Player::respawnPlayer()
{
	if (m_pickupPointer)
	{
		if (m_pickupPointer->isActive())
		{
			m_pickupPointer->onDepleted();
			m_pickupPointer->onRemove();
			SAFE_DELETE(m_pickupPointer);

		}
	}

	m_state = PlayerState::IDLE;
	m_controller->setPosition(m_checkpointPos);
}

float Player::getPlayerScale() const
{
	return this->m_playerScale;
}

int Player::getScore()
{
	return m_score;
}

void Player::setScore(int newScore)
{
	m_score = newScore;
	GUIHandler::get().changeGUIText(m_scoreGUIIndex, std::to_string(m_score));
}

Entity* Player::getPlayerEntity() const
{
	return m_playerEntity;
}

Vector3 Player::getCameraOffset()
{
	return m_cameraOffset;
}

const bool Player::canUsePickup()
{
	return m_pickupPointer && !m_pickupPointer->isActive();
}

void Player::handlePickupOnUse()
{
	PlayerMessageData data;
	data.playerActionType = PlayerActions::ON_POWERUP_USE;
	data.intEnum = (int)m_pickupPointer->getPickupType();
	data.playerPtr = this;
	switch (m_pickupPointer->getPickupType())
	{
	case PickupType::SPEED:
		//Not used since it is activated on use.

		//m_currentSpeedModifier = 1.f;
		//m_goalSpeedModifier = m_pickupPointer->getModifierValue();
		//m_speedModifierTime = 0;
		break;
	case PickupType::HEIGHTBOOST:
		//Kalla p� animation
		jump(false);
		break;
	case PickupType::CANNON:
		m_state = PlayerState::CANNON;
		m_cameraOffset = Vector3(1.f, 2.5f, 0.f);
		//Cannon on use
	default:
		break;
	}
	m_pickupPointer->onUse();
	for (int i = 0; i < m_playerObservers.size(); i++)
	{
		m_playerObservers[i]->reactOnPlayer(data);
	}
}

void Player::inputUpdate(InputData& inputData)
{

	if (m_state == PlayerState::CANNON)
	{
		for (std::vector<int>::size_type i = 0; i < inputData.actionData.size(); i++)
		{
			if (inputData.actionData.at(i) == Action::USE)
			{
				m_shouldFire = true;
			}

		}
	}


	this->setStates(inputData.stateData);

	for (std::vector<int>::size_type i = 0; i < inputData.actionData.size(); i++)
	{
		switch (inputData.actionData[i])
		{
		case JUMP:
			if (m_state == PlayerState::IDLE || ((m_state == PlayerState::JUMPING || m_state == PlayerState::FALLING) && m_jumps < ALLOWED_NR_OF_JUMPS))
				jump();
			break;
		case DASH:
			if (m_state == PlayerState::IDLE)
			{
				if (canRoll())
					roll();
			}
			else
			{
				if (canDash())
					dash();
			}
			break;
		case USEPICKUP:
			if (canUsePickup())
				handlePickupOnUse();
			break;
		case CLOSEINTROGUI:
			GUIHandler::get().setVisible(m_instructionGuiIndex, false);
			GUIHandler::get().setVisible(closeInstructionsBtnIndex, false);
			break;
		default:
			break;
		}
	}
}

void Player::sendPhysicsMessage(PhysicsData& physicsData, bool &shouldTriggerEntityBeRemoved)
{
	
	if (!shouldTriggerEntityBeRemoved)
	{
		//Traps
		if (physicsData.triggerType == TriggerType::TRAP)
		{
			switch ((TrapType)physicsData.associatedTriggerEnum)
			{
			case TrapType::SLOW:
				m_activeTrap = (TrapType)physicsData.associatedTriggerEnum;
				m_currentSpeedModifier = 0.5f;
				m_goalSpeedModifier = physicsData.floatData;
				m_speedModifierTime = 0;
				break;
			}

		}

		if (physicsData.triggerType == TriggerType::BARREL)
		{

			//spelare - barrel
			/*	Vector3 direction = ->getTranslation() - m_playerEntity->getTranslation();
			direction.Normalize();
			m_playerEntity->translate(direction);*/

			jump();
		}

		//Checkpoints
		if (physicsData.triggerType == TriggerType::CHECKPOINT)
		{
			Entity* ptr = static_cast<Entity*>(physicsData.pointer);

			CheckpointComponent* checkpointPtr = dynamic_cast<CheckpointComponent*>(ptr->getComponent("checkpoint"));
			if (!checkpointPtr->isUsed())
			{
				AudioComponent* audioPtr = dynamic_cast<AudioComponent*>(ptr->getComponent("sound"));
				audioPtr->playSound();

				m_checkpointPos = ptr->getTranslation();

				checkpointPtr->setUsed(true);
			}
		}

		//Pickup 
		if (physicsData.triggerType == TriggerType::PICKUP)
		{
			/*
				assosiatedTriggerEnum - PickupType
				intData - used currently for heightBoost to check if they use trampoline object or if they pickedup the item(intData = fromPickup
				stringData -free
				floatData - modifier for speedboost.
			*/
			
			if (m_pickupPointer == nullptr || ((bool)!physicsData.intData && (PickupType)physicsData.associatedTriggerEnum == PickupType::HEIGHTBOOST ))
			{
				bool environmenPickup = false;
				bool addPickupByAssosiatedID = true; // If we do not want to add pickup change this to false in switchCase.
				switch ((PickupType)physicsData.associatedTriggerEnum)
				{
				case PickupType::SPEED:
					shouldTriggerEntityBeRemoved = true; //We want to remove speedpickup entity after we've used it.
					m_currentSpeedModifier = 1.f;
					m_goalSpeedModifier = physicsData.floatData;
					m_speedModifierTime = 0;
					break;
				case PickupType::HEIGHTBOOST:
					if ((bool)physicsData.intData) //fromPickup -true/false
					{
						shouldTriggerEntityBeRemoved = true;
					}	
					else
					{
						jump(false, 1.5f);
						environmenPickup = true;
					}


					break;
				case PickupType::CANNON:
					shouldTriggerEntityBeRemoved = true;
					break;
				case PickupType::SCORE:
					addPickupByAssosiatedID = false;
					break;
				default:
					break;
				}
				if (addPickupByAssosiatedID)
				{
					Pickup* pickupPtr = getCorrectPickupByID(physicsData.associatedTriggerEnum);
					pickupPtr->setModifierValue(physicsData.floatData);


					if (environmenPickup)
					{

						if (m_environmentPickup) //If we already have an environmentpickup (SInce they can be force added, we need to, if it exist, remove the "old" enironmentPickup before creating/getting a new.
						{
							m_environmentPickup->onRemove();
							delete m_environmentPickup;
							m_environmentPickup = nullptr;
						}
						pickupPtr->onPickup(m_playerEntity);
						pickupPtr->onUse();
						m_environmentPickup = pickupPtr;
					}
					else
					{
						m_pickupPointer = pickupPtr;
						pickupPtr->onPickup(m_playerEntity);
						if (pickupPtr->shouldActivateOnPickup())
							pickupPtr->onUse();
					}
				}
			}
			//Score
				if((PickupType)physicsData.associatedTriggerEnum == PickupType::SCORE)
			{
				int amount = (int)physicsData.floatData;
				this->increaseScoreBy(amount);
				m_audioComponent->playSound();
				shouldTriggerEntityBeRemoved = true;
			}

		}
	}
}

Pickup* getCorrectPickupByID(int id)
{
	Pickup* pickupPtr = Pickup::getPickupByID(id);
	Pickup* createPickup = nullptr;
	switch (pickupPtr->getPickupType())
	{
	case PickupType::SPEED:
		createPickup = new SpeedPickup(*dynamic_cast<SpeedPickup*>(pickupPtr));
		break;
	case PickupType::HEIGHTBOOST:
		createPickup = new HeightPickup(*dynamic_cast<HeightPickup*>(pickupPtr));
		break;
	case PickupType::CANNON:
		createPickup = new CannonPickup(*dynamic_cast<CannonPickup*>(pickupPtr));
		break;
	case PickupType::SCORE: //Score is not saved as a pointer so not implemented.
		break;
	default:
		break;
	}

	return createPickup;
}

void Player::update(GUIUpdateType type, GUIElement* guiElement)
{
	if (type == GUIUpdateType::CLICKED)
	{
		GUIHandler::get().setVisible(guiElement->m_index, false);

		//Set instructions to not visible
		GUIHandler::get().setVisible(m_instructionGuiIndex, false);
	}


	if (type == GUIUpdateType::HOVER_ENTER)
	{
		GUIButton* button = dynamic_cast<GUIButton*>(guiElement);
		button->setTexture(L"closeButtonSelected.png");
	}

	if (type == GUIUpdateType::HOVER_EXIT)
	{
		GUIButton* button = dynamic_cast<GUIButton*>(guiElement);
		button->setTexture(L"closeButton.png");
	}
}
void Player::serverPlayerAnimationChange(PlayerState currentState, float currentBlend)
{
	m_animMesh->setCurrentBlend(currentBlend);

	if (currentState != m_state)
	{
		m_state = currentState;

		switch (currentState)
		{
		case PlayerState::IDLE:
			idleAnimation();
			break;
		case PlayerState::JUMPING:

			break;
		case PlayerState::FALLING:

			break;
		case PlayerState::DASH:
			dashAnimation();
			break;
		case PlayerState::ROLL:
			rollAnimation();
			break;
		}
	}
}

void Player::jump(const bool& incrementCounter, const float& multiplier)
{
	m_currentDistance = 0;
	m_state = PlayerState::JUMPING;
	m_velocity.y = JUMP_SPEED * m_playerScale * multiplier;// * dt;
	if(incrementCounter)
		m_jumps++;
}

bool Player::canRoll() const
{
	return (m_state == PlayerState::IDLE);
}

void Player::roll()
{
	prepDistVariables();
	m_controller->setControllerSize(ROLL_HEIGHT);
	m_controller->setControllerRadius(ROLL_RADIUS);
	m_state = PlayerState::ROLL;

	rollAnimation();
}

bool Player::canDash() const
{
	return (m_state == PlayerState::JUMPING || m_state == PlayerState::FALLING && !m_hasDashed);
}

void Player::dash()
{
	prepDistVariables();
	m_state = PlayerState::DASH;

	dashAnimation();
}

void Player::prepDistVariables()
{
	m_currentDistance = 0;
	m_moveDirection = m_playerEntity->getForwardVector();
}

void Player::rollAnimation()
{
	m_animMesh->playSingleAnimation("platformer_guy_roll1", 0.1f, false);
	m_animMesh->setAnimationSpeed(1.6f);
}

void Player::dashAnimation()
{
	m_animMesh->playSingleAnimation("platformer_guy_pose", 0.1f, true);
}

void Player::idleAnimation()
{
	m_animMesh->playBlendState("runOrIdle", 0.3f);
}
