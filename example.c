#define ECS_IMPLEMENTATION
//#define ECS_ENABLE_LOGGING
#include "ecs.h"

#include <math.h>

//Define our components
typedef struct
{
    float x, y;
} CTransform;		// a position component

typedef struct
{
    float dx, dy;
} CVelocity;		// a velocity component

typedef struct
{
    unsigned int gl_id;
    float rotation;
    const char* name;
} CSprite;			// a sprite component including a "name"

typedef struct
{
    EcsEnt target;
} Ctarget;			// target component if you have this then
					// the entity is a missile

typedef enum
{
    COMPONENT_TRANSFORM,
    COMPONENT_VELOCITY,
    COMPONENT_SPRITE,
    COMPONENT_TARGET,

    COMPONENT_COUNT
} ComponentType;

//ECS_MASK takes the number of components, and then the components
//if you only need to check if the entity has one component,
//you can optionally use ecs_ent_has_component

#define MOVEMENT_SYSTEM_MASK \
ECS_MASK(2, COMPONENT_TRANSFORM, COMPONENT_VELOCITY)

// an entity with a transform and velocity system
// moves....
void
movement_system(Ecs *ecs)
{
    for (unsigned int i = 0; i < ecs_for_count(ecs); i++)
    {
        EcsEnt e = ecs_get_ent(ecs, i);
        if (ecs_ent_has_mask(ecs, e, MOVEMENT_SYSTEM_MASK))
        {
            CTransform *xform   = ecs_ent_get_component(ecs, e, COMPONENT_TRANSFORM);
            CVelocity *velocity = ecs_ent_get_component(ecs, e, COMPONENT_VELOCITY);

            xform->x += velocity->dx;
            xform->y += velocity->dy;
        }
    }
}


#define SPRITE_RENDER_SYSTEM_MASK \
ECS_MASK(2, COMPONENT_TRANSFORM, COMPONENT_SPRITE)

// an entity with a sprite and transform gets "rendered"
void
sprite_render_system(Ecs *ecs)
{
    for (unsigned int i = 0; i < ecs_for_count(ecs); i++)
    {
        EcsEnt e = ecs_get_ent(ecs, i);
        if (ecs_ent_has_mask(ecs, e, SPRITE_RENDER_SYSTEM_MASK))
        {
			CTransform *xform = ecs_ent_get_component(ecs, e, COMPONENT_TRANSFORM);
			CSprite *sprite   = ecs_ent_get_component(ecs, e, COMPONENT_SPRITE);
			printf("id %i (%s), rot %f, x %f, y %f\n", sprite->gl_id, sprite->name, 
												sprite->rotation, xform->x, xform->y);
        }
    }
}

#define MISSILE_SYSTEM_MASK \
ECS_MASK(2, COMPONENT_TRANSFORM, COMPONENT_TARGET)

// this moves a missile towards it target
// an entity is a missile if it has a target component
void
missile_system(Ecs *ecs)
{
    for (unsigned int i = 0; i < ecs_for_count(ecs); i++)
    {
        EcsEnt e = ecs_get_ent(ecs, i);
        if (ecs_ent_has_mask(ecs, e, MISSILE_SYSTEM_MASK))
        {

            Ctarget *missile   = ecs_ent_get_component(ecs, e, COMPONENT_TARGET);

            //when storing a reference to an EcsEnt we must check if the entity is valid before
            //operating on it
            if (ecs_ent_is_valid(ecs, missile->target))
            {
                if (ecs_ent_has_component(ecs, missile->target, COMPONENT_TRANSFORM)) {
                    CTransform *xform = ecs_ent_get_component(ecs, e, COMPONENT_TRANSFORM);
                    CVelocity *vel = ecs_ent_get_component(ecs, e, COMPONENT_VELOCITY);
                    CTransform *TargXform = ecs_ent_get_component(ecs, missile->target, COMPONENT_TRANSFORM);
                    float tx = (TargXform->x - xform->x);
                    float ty = (TargXform->y - xform->y);
                    float d = sqrtf(tx*tx + ty*ty);
                    // we could just directly change the position
                    // or alter its velocity...
                    vel->dx = (tx / d) / 4.0;
                    vel->dy = (ty / d) / 4.0;
                    printf("target distance %f\n",d);
                    if (d < 0.2) {
						// if the missile gets close enough to its
						// target then missile and target are destroyed
                        printf("BOOM!\n");
                        render_sprite(ecs, e);
                        render_sprite(ecs, missile->target);
                        ecs_ent_destroy(ecs, e);
                        ecs_ent_destroy(ecs, missile->target);
                    }
                } else {
					// shouldn't MISSILE_SYSTEM_MASK prevent this ???
                    printf("missile target has no transform\n");
                }
            }
            //or maybe just remove the component if its not valid, all depends on the situation
        }
    }
}

void
register_components(Ecs *ecs)
{
    //Ecs, component index, component pool size, size of component, and component free func
    ecs_register_component(ecs, COMPONENT_TRANSFORM, 1000, sizeof(CTransform), NULL);
    ecs_register_component(ecs, COMPONENT_VELOCITY, 200, sizeof(CVelocity), NULL);
    ecs_register_component(ecs, COMPONENT_SPRITE, 1000, sizeof(CSprite), NULL);
    ecs_register_component(ecs, COMPONENT_TARGET, 10, sizeof(Ctarget), NULL);
}

void
register_systems(Ecs *ecs)
{
    //ecs_run_systems will run the systems in the order they are registered
    //ecs_run_system is also available if you wish to handle each system seperately
    //
    //Ecs, function pointer to system (must take a parameter of Ecs), system type
    ecs_register_system(ecs, movement_system, ECS_SYSTEM_UPDATE);
    ecs_register_system(ecs, missile_system, ECS_SYSTEM_UPDATE);
    ecs_register_system(ecs, sprite_render_system, ECS_SYSTEM_RENDER);
}

int countEnts(Ecs *ecs)
{
    // assume if an entity doesn't have a transform component then
    // it isn't active
    int count = 0;
    for (unsigned int i = 0; i < ecs_for_count(ecs); i++)
    {
        EcsEnt e = ecs_get_ent(ecs, i);
        if (ecs_ent_has_component(ecs, e, COMPONENT_TRANSFORM)) count++;
    }
    return count;
}

int
main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    //Max entities, component count, system_count
    Ecs *ecs = ecs_make(1000, COMPONENT_COUNT, 3);
    register_components(ecs);
    register_systems(ecs);

    EcsEnt e = ecs_ent_make(ecs);
    CTransform xform = {0, 0};
    CVelocity velocity = { .1, 0};
    CSprite sprite = { 1, 0, "ship" };
    ecs_ent_add_component(ecs, e, COMPONENT_TRANSFORM, &xform);
    ecs_ent_add_component(ecs, e, COMPONENT_VELOCITY, &velocity);
    ecs_ent_add_component(ecs, e, COMPONENT_SPRITE, &sprite);


    EcsEnt e2 = ecs_ent_make(ecs);
    CTransform misPos = {4, 4, "missile"};
    Ctarget misTarg = { e };
    CSprite misSprite = { 2, 0 };
    CVelocity misVel = { 0, 0 };
    ecs_ent_add_component(ecs, e2, COMPONENT_TRANSFORM, &misPos);
    ecs_ent_add_component(ecs, e2, COMPONENT_VELOCITY, &misVel);
    ecs_ent_add_component(ecs, e2, COMPONENT_SPRITE, &misSprite);
    ecs_ent_add_component(ecs, e2, COMPONENT_TARGET, &misTarg);

    ecs_ent_print(ecs, e);
    ecs_ent_print(ecs, e2);

    //main loop code
    while(countEnts(ecs))
    {
        ecs_run_systems(ecs, ECS_SYSTEM_UPDATE);
        ecs_run_systems(ecs, ECS_SYSTEM_RENDER);
        printf("------------------\n");
    }

    ecs_destroy(ecs);
}
