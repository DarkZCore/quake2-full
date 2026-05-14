#include "g_local.h"

static void Helper_Think(edict_t* self);
static void Helper_Touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf);
static void Helper_Die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point);
static void Helper_Shoot(edict_t* self, edict_t* enemy);

static void Helper_Touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
    return;
}


static void Helper_Die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(TE_EXPLOSION1);
    gi.WritePosition(self->s.origin);
    gi.multicast(self->s.origin, MULTICAST_PVS);

    G_FreeEdict(self);
}


void SP_monster_helper(edict_t* self)
{
    self->movetype = MOVETYPE_STEP;
    self->solid = SOLID_BBOX;
    self->s.modelindex = gi.modelindex("models/objects/rocket/tris.md2"); // placeholder model
    self->mins[0] = self->mins[1] = -16;
    self->mins[2] = -16;
    self->maxs[0] = self->maxs[1] = 16;
    self->maxs[2] = 16;

    self->health = 100;
    self->max_health = 100;
    self->takedamage = DAMAGE_YES;
    self->die = Helper_Die;
    self->touch = Helper_Touch;
    self->think = Helper_Think;
    self->nextthink = level.time + FRAMETIME;

    self->classname = "monster_helper";
    gi.linkentity(self);
}

static void Helper_Shoot(edict_t* self, edict_t* enemy)
{
    vec3_t start, dir, forward, right, offset;
    trace_t tr;

    VectorSubtract(enemy->s.origin, self->s.origin, dir);
    VectorNormalize(dir);

    vectoangles(dir, dir);
    AngleVectors(dir, forward, right, NULL);

    VectorSet(offset, 20, 0, 10);
    G_ProjectSource(self->s.origin, offset, forward, right, start);

    monster_fire_blaster(self, start, forward, 15, 1000, 0, EF_BLASTER);
}


static void Helper_Think(edict_t* self)
{
    edict_t* enemy = NULL;
    edict_t* ent;
    vec3_t v;
    float bestdist = 99999;
    int i;

    if (!self->owner || !self->owner->inuse)
    {
        G_FreeEdict(self);
        return;
    }

    // follow owner
    VectorSubtract(self->owner->s.origin, self->s.origin, v);
    if (VectorLength(v) > 80)
    {
        VectorNormalize(v);
        VectorScale(v, 200, self->velocity);
        self->velocity[2] = 0;
    }
    else
    {
        VectorClear(self->velocity);
    }

    // find nearest enemy
    for (i = 1; i <= maxclients->value; i++)
    {
        ent = &g_edicts[i];
        if (!ent->inuse || !ent->client)
            continue;
        if (ent == self->owner)
            continue;
    }

    ent = NULL;
    while ((ent = findradius(ent, self->s.origin, 600)) != NULL)
    {
        if (!ent->takedamage || ent == self || ent == self->owner)
            continue;
        if (!CanDamage(ent, self))
            continue;

        VectorSubtract(ent->s.origin, self->s.origin, v);
        if (VectorLength(v) < bestdist)
        {
            bestdist = VectorLength(v);
            enemy = ent;
        }
    }

    if (enemy)
        Helper_Shoot(self, enemy);

    // heal owner occasionally
    if (level.time > self->wait && self->owner->health < self->owner->max_health)
    {
        self->owner->health += 5;
        if (self->owner->health > self->owner->max_health)
            self->owner->health = self->owner->max_health;
        self->wait = level.time + 2.0f;
    }

    // give ammo occasionally
    if (level.time > self->delay)
    {
        gitem_t* ammo = FindItem("bullets");
        if (ammo)
            Add_Ammo(self->owner, ammo, 5);
        self->delay = level.time + 4.0f;
    }

    // self-shield occasionally
    if (level.time > self->teleport_time)
    {
        self->flags |= FL_GODMODE;
        self->teleport_time = level.time + 1.5f;
    }
    else
    {
        self->flags &= ~FL_GODMODE;
    }

    self->nextthink = level.time + FRAMETIME;
    gi.linkentity(self);
}
