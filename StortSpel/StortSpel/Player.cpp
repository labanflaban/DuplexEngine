#include "3DPCH.h"
#include "Player.h"
#include"Pickup.h"
#include"SpeedPickup.h"
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
		Pickup::initPickupArray(vec);
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

	//GUIImageStyle imageStyle;
	//imageStyle.position = Vector2(400.f, 50.f);
	//imageStyle.scale = Vector2(0.9f, 0.9f);
	//m_instructionGuiIndex = GUIHandler::get().addGUIImage(L"keyboard.png", imageStyle);

	////Test Button stuff
	//GUIButtonStyle btnStyle;
	//btnStyle.position = Vector2(240, 200);
	//btnStyle.scale = Vector2(0.5, 0.5);
	//closeInstructionsBtnIndex = GUIHandler::get().addGUIButton(L"closeButton.png", btnStyle);

	//Attach to the click listener for the button
	//dynamic_cast<GUIButton*>(GUIHandler::get().getElementMap()->at(closeInstructionsBtnIndex))->Attach(this);
}

Player::~Player()
{
	if (m_playerEntity)
		delete m_playerEntity;
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
	default:
		break;
	}

	if (m_state == PlayerState::FALLING || m_state == PlayerState::JUMPING || m_state == PlayerState::ROLL)
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

void Player::updatePlayer(const float& dt)
{
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


	if (m_pickupPointer)
	{
		if (m_pickupPointer->isActive())
		{
			switch (m_pickupPointer->getPickupType())
			{
			case PickupType::SPEED:
				m_speedModifierTime += dt;
				if (m_speedModifierTime <= FOR_FULL_EFFECT_TIME)
				{
					m_currentSpeedModifier += m_goalSpeedModifier * dt / FOR_FULL_EFFECT_TIME;
				}
				m_pickupPointer->update(dt);
				if (m_pickupPointer->isDepleted())
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

					if (m_pickupPointer->shouldDestroy())
					{
						m_pickupPointer->onRemove();
						delete m_pickupPointer;
						m_pickupPointer = nullptr;
						m_currentSpeedModifier = 1.f;
					}
				}
				break;
			case PickupType::SCORE:

				break;
			default:
				break;
			}
		}

	}

	if(m_state != PlayerState::ROLL)
		handleRotation(dt);

	playerStateLogic(dt);

	ImGui::Begin("Player Information", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
	ImGui::Text("Player Position: (%d %d, %d)", (int)this->getPlayerEntity()->getTranslation().x, (int)this->getPlayerEntity()->getTranslation().y, (int)this->getPlayerEntity()->getTranslation().z);
	ImGui::End();
}

void Player::setPlayerEntity(Entity* entity)
{
	m_playerEntity = entity;
	m_controller = static_cast<CharacterControllerComponent*>(m_playerEntity->getComponent("CCC"));
	entity->addComponent("ScoreAudio", m_audioComponent = new AudioComponent(m_scoreSound));
}

Vector3 Player::getCheckpointPos()
{
	return m_checkpointPos;
}

Vector3 Player::getVelocity()
{
	return m_velocity;
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

const bool Player::canUsePickup()
{
	return m_pickupPointer && !m_pickupPointer->isActive();
}

void Player::handlePickupOnUse()
{
	switch (m_pickupPointer->getPickupType())
	{
	case PickupType::SPEED:
		//Not used since it is activated on use.

		//m_currentSpeedModifier = 1.f;
		//m_goalSpeedModifier = m_pickupPointer->getModifierValue();
		//m_speedModifierTime = 0;
		break;
	default:
		break;
	}

	m_pickupPointer->onUse();
}

void Player::inputUpdate(InputData& inputData)
{
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
	if (!shouldTriggerEntityBeRemoved)
	{

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

		if (physicsData.triggerType == TriggerType::PICKUP)
		{
			if (m_pickupPointer == nullptr)
			{
				bool addPickupByAssosiatedID = true; // If we do not want to add pickup change this to false in switchCase.
				float duration = (float)physicsData.intData;
				switch ((PickupType)physicsData.associatedTriggerEnum)
				{
				case PickupType::SPEED:
					shouldTriggerEntityBeRemoved = true;
					m_currentSpeedModifier = 1.f;
					m_goalSpeedModifier = physicsData.floatData;
					m_speedModifierTime = 0;
					break;
				case PickupType::SCORE:
					addPickupByAssosiatedID = false;
					break;
				default:
					break;
				}
				if (addPickupByAssosiatedID)
				{
					m_pickupPointer = getCorrectPickupByID(physicsData.associatedTriggerEnum);
					m_pickupPointer->setModifierValue(physicsData.floatData);
					m_pickupPointer->onPickup(m_playerEntity, duration);
					if (m_pickupPointer->shouldActivateOnPickup())
						m_pickupPointer->onUse();
				}
			}

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

void Player::jump()
{
	m_currentDistance = 0;
	m_state = PlayerState::JUMPING;
	m_velocity.y = JUMP_SPEED * m_playerScale;// * dt;
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
	m_animMesh->playSingleAnimation("platformer_guy_roll1", 0.1f, false, true);
	m_animMesh->setAnimationSpeed(1.6f);
}

void Player::dashAnimation()
{
	m_animMesh->playSingleAnimation("platformer_guy_pose", 0.1f, true, true);
}

void Player::idleAnimation()
{
	m_animMesh->playBlendState("runOrIdle", 0.3f);
}
