extern concptr KWD_DAM;
extern concptr KWD_RANGE;
extern concptr KWD_DURATION;
extern concptr KWD_SPHERE;
extern concptr KWD_HEAL;
extern concptr KWD_RANDOM;

extern concptr info_string_dice(concptr str, DICE_NUMBER dice, DICE_SID sides, int base);
extern concptr info_damage(DICE_NUMBER dice, DICE_SID sides, int base);
extern concptr info_duration(int base, DICE_SID sides);
extern concptr info_range(POSITION range);
extern concptr info_heal(DICE_NUMBER dice, DICE_SID sides, int base);
extern concptr info_delay(int base, DICE_SID sides);
extern concptr info_multi_damage(HIT_POINT dam);
extern concptr info_multi_damage_dice(DICE_NUMBER dice, DICE_SID sides);
extern concptr info_power(int power);
extern concptr info_power_dice(DICE_NUMBER dice, DICE_SID sides);
extern concptr info_radius(POSITION rad);
extern concptr info_weight(WEIGHT weight);

/* cmd5.c */
extern void do_cmd_browse(void);
extern void do_cmd_study(void);
extern void do_cmd_cast(void);
